#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer.hpp"
#include "game/Physics.hpp"

#define NUM_DEPTH_PEEL_PASSES 4

glm::mat4 getProjMat(ecs::Entity_t const &entity) 
{
    game::Camera &camera = ecs::get<game::Camera>(entity);
    if(ecs::entityHasComponent<game::PerspectiveProjection>(entity))
        return glm::perspective<float>(glm::radians(camera.fov), (float) camera.width / camera.height, camera.znear, camera.zfar);
    else
        return glm::ortho<float>((float) -camera.width / camera.height, (float) camera.width / camera.height, -1, 1, camera.znear, camera.zfar); // FIXME: smashed random shit here
}
glm::mat4 getViewMat(ecs::Entity_t const &entity) 
{
    using namespace game;
    if(ecs::entityHasComponent<RotationQuaternion>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<Position>(entity) ? ecs::get<Position>(entity).position : glm::vec3{0, 0, 0};
        return glm::translate(glm::mat4_cast(glm::normalize(ecs::get<RotationQuaternion>(entity).quat)), -position);
    } else if(ecs::entityHasComponent<Rotation>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<Position>(entity) ? ecs::get<Position>(entity).position : glm::vec3{0, 0, 0};
        glm::vec3 orientation = glm::radians(ecs::get<Rotation>(entity).rotation);
        glm::vec3 forward = glm::normalize(glm::vec3(
            cos(orientation.y) * cos(orientation.x),
            sin(orientation.x),
            sin(orientation.y) * cos(orientation.x)
        ));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::vec3{
            glm::rotate(glm::mat4{1.0f}, orientation.z, {0, 0, 1}) * glm::vec4{glm::cross(right, forward), 0}
        });
        right = glm::cross(forward, up);
        
        return glm::lookAt(position, position + forward, up);
    } else if(ecs::entityHasComponent<Position>(entity)) {
        return glm::translate(glm::mat4{1.0f}, -ecs::get<Position>(entity).position);
    } else {
        return glm::mat4{1.0f};
    }
}
glm::mat4 getModelMat(ecs::Entity_t const &entity) 
{
    glm::mat4 modelMat{1.0f};
    if(ecs::entityHasComponent<game::ModelMatrix>(entity)) {
        modelMat = ecs::get<game::ModelMatrix>(entity).modelMatrix;
    }
    if(ecs::entityHasComponent<game::Position>(entity)) {
        modelMat = glm::translate(modelMat, ecs::get<game::Position>(entity).position);
    }
    if(ecs::entityHasComponent<game::Rotation>(entity)) {
        glm::vec3 const &rotation = ecs::get<game::Rotation>(entity).rotation;
        modelMat = glm::rotate<float>(modelMat, rotation.x, {1, 0, 0});
        modelMat = glm::rotate<float>(modelMat, rotation.y, {0, 1, 0});
        modelMat = glm::rotate<float>(modelMat, rotation.z, {0, 0, 1});
    }
    if(ecs::entityHasComponent<game::Scale>(entity)) {
        modelMat = glm::scale(modelMat, ecs::get<game::Scale>(entity).scale);
    }
    return modelMat;
}
void draw(game::Drawable const &drawable) {
    drawable.va.bind();
    if(drawable.ib.has_value()) {
        drawable.ib.value().bind();
        glDrawElements(drawable.mode, drawable.count, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(drawable.mode, 0, drawable.count);
    }
}

void game::Renderer::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &cameraEntity : entities) {
        if(!ecs::entityHasComponent<Camera>(cameraEntity) || !ecs::entityHasComponent<RenderTarget>(cameraEntity)) continue;
        Camera &camera = ecs::get<Camera>(cameraEntity);
        RenderTarget &rtarget = ecs::get<RenderTarget>(cameraEntity);

        if(rtarget.prevWidth != camera.width || rtarget.prevHeight != camera.height) { // resize or initialize buffers / textures
            rtarget.OIT_accumTexture.bind();  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, camera.width, camera.height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
            rtarget.OIT_opaqueTexture.bind(); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, camera.width, camera.height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
            rtarget.OIT_revealTexture.bind(); glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, camera.width, camera.height, 0, GL_RED, GL_HALF_FLOAT, nullptr);
            rtarget.OIT_depthTexture.bind();  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, camera.width, camera.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        if(rtarget.prevWidth == -1) { // initialize render target
            rtarget.OIT_opaqueFBO.bind();
            rtarget.OIT_opaqueFBO.attach(rtarget.OIT_opaqueTexture, GL_COLOR_ATTACHMENT0);
            rtarget.OIT_opaqueFBO.attach(rtarget.OIT_depthTexture,  GL_DEPTH_ATTACHMENT);
            assert(rtarget.OIT_opaqueFBO.isComplete());

            rtarget.OIT_transparentFBO.bind();
            rtarget.OIT_transparentFBO.attach(rtarget.OIT_accumTexture, GL_COLOR_ATTACHMENT0);
            rtarget.OIT_transparentFBO.attach(rtarget.OIT_revealTexture, GL_COLOR_ATTACHMENT1);
            GLenum const transparentDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glDrawBuffers(2, transparentDrawBuffers);
            assert(rtarget.OIT_transparentFBO.isComplete());
        }
        rtarget.prevWidth = camera.width;
        rtarget.prevHeight = camera.height;
        
        camera.projMat = getProjMat(cameraEntity);
        camera.viewMat = getViewMat(cameraEntity);
        glm::vec3 cameraPosition = glm::vec4{0, 0, 0, 1} * camera.viewMat;
        
        glBindFramebuffer(GL_FRAMEBUFFER, rtarget.mainFBOid);
        glViewport(0, 0, camera.width, camera.height);
        glClearColor(rtarget.clearColor.r, rtarget.clearColor.g, rtarget.clearColor.b, rtarget.clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        glm::vec4 zeroCleaner{0};
        glm::vec4 oneCleaner{1};
        rtarget.OIT_transparentFBO.bind();
        glClearBufferfv(GL_COLOR, 0, &zeroCleaner.x);
        glClearBufferfv(GL_COLOR, 1, &oneCleaner.x);

        rtarget.OIT_opaqueFBO.bind();
        glClearColor(rtarget.clearColor.r, rtarget.clearColor.g, rtarget.clearColor.b, rtarget.clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for(ecs::Entity_t const &entity : entities) { // TODO: batch it or something?
            if(ecs::entityHasComponent<Drawable>(entity)) {
                Drawable &drawable = ecs::get<Drawable>(entity);
                glm::mat4 modelMat = getModelMat(entity);

                if(ecs::entityHasComponent<Transparent>(entity)) {
                    glDepthMask(GL_FALSE);
                    glEnable(GL_BLEND);
                    glBlendFunci(0, GL_ONE, GL_ONE);
                    glBlendFunci(1, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBlendEquation(GL_FUNC_ADD);
                    rtarget.OIT_transparentFBO.bind();
                    m_transparentShader.bind();

                    glUniformMatrix4fv(m_transparentShader.getUniform("u_modelMat"), 1, GL_FALSE, &modelMat[0][0]);
                    glUniformMatrix4fv(m_transparentShader.getUniform("u_viewMat"), 1, GL_FALSE, &camera.viewMat[0][0]);
                    glUniformMatrix4fv(m_transparentShader.getUniform("u_projectionMat"), 1, GL_FALSE, &camera.projMat[0][0]);
                    glUniform3fv(      m_transparentShader.getUniform("u_cameraPosition"), 1, &cameraPosition.x);
                    glUniform1i(       m_transparentShader.getUniform("u_texture"), 0);
                    if(ecs::entityHasComponent<opengl::Texture>(entity)) {
                        ecs::get<opengl::Texture>(entity).bind(0);
                    } else {
                        m_whiteTexture.bind(0);
                    }
                    if(ecs::entityHasComponent<Color>(entity)) {
                        glUniform4fv(m_transparentShader.getUniform("u_color"), 1, &ecs::get<Color>(entity).color.r);
                    } else {
                        glUniform4f(m_transparentShader.getUniform("u_color"), 1, 1, 1, 1);
                    }
                    draw(drawable);
                } else {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);
                    rtarget.OIT_opaqueFBO.bind();
                    drawable.shader->bind();
                    if(ecs::entityHasComponent<Color>(entity)) {
                        glUniform4fv(drawable.shader->getUniform("u_color"), 1, &ecs::get<Color>(entity).color.r);
                    } else {
                        glUniform4f(drawable.shader->getUniform("u_color"), 1, 1, 1, 1);
                    }
                    glUniform1i(drawable.shader->getUniform("u_texture"), 0);
                    if(ecs::entityHasComponent<opengl::Texture>(entity)) {
                        ecs::get<opengl::Texture>(entity).bind(0);
                    } else {
                        m_notfound.bind(0);
                    }
                    glUniformMatrix4fv(drawable.shader->getUniform("u_modelMat"), 1, GL_FALSE, &modelMat[0][0]);
                    glUniformMatrix4fv(drawable.shader->getUniform("u_viewMat"), 1, GL_FALSE, &camera.viewMat[0][0]);
                    glUniformMatrix4fv(drawable.shader->getUniform("u_projectionMat"), 1, GL_FALSE, &camera.projMat[0][0]);
                    glUniform3fv(      drawable.shader->getUniform("u_cameraPosition"), 1, &cameraPosition.x);
                    draw(drawable);
                }
            }

            // ========================================
            if(ecs::entityHasComponent<Text>(entity)) {
                Text const &text = ecs::get<Text>(entity);
                glm::vec4 color = ecs::entityHasComponent<Color>(entity) ? ecs::get<Color>(entity).color : glm::vec4{0.5, 0.5, 0.5, 1};
                glm::mat4 matrix = text.matrix.value_or(glm::mat4{1.0f});
                glBindFramebuffer(GL_FRAMEBUFFER, rtarget.mainFBOid);
                text.font->drawText(text.text, text.position, text.size, color, matrix);
            }
        } // for(auto &entity : entities) 

        // oit composite pass
        // ==================
        glDepthFunc(GL_ALWAYS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        rtarget.OIT_opaqueFBO.bind();
        m_oitCompositeShader.bind();
        opengl::VertexArray emptyVAO{{}, opengl::VertexBufferLayout{}};
        emptyVAO.bind();
        glUniform1i(m_oitCompositeShader.getUniform("u_accum"), 0);  rtarget.OIT_accumTexture.bind(0);
        glUniform1i(m_oitCompositeShader.getUniform("u_reveal"), 1); rtarget.OIT_revealTexture.bind(1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // main FBO pass
        // =============
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glClearColor(0, 0, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, rtarget.mainFBOid);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_hdrShader.bind();
        emptyVAO.bind();
        rtarget.OIT_opaqueTexture.bind(0);
        glUniform1i(m_hdrShader.getUniform("u_texture"), 0);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } // for(auto &cameraEntity : entities)
}
