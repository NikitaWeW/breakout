#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer.hpp"
#include "Animator.hpp"
#include "game/Physics.hpp"
#include "utils/Model.hpp"

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
    } else if(ecs::entityHasComponent<game::RotationQuaternion>(entity)) {
        modelMat = modelMat * glm::mat4_cast(ecs::get<game::RotationQuaternion>(entity).quat);
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
void drawText(ecs::Entity_t const &textEntity) {
    using namespace game;
    assert(ecs::entityHasComponent<Text>(textEntity));
    Text const &text = ecs::get<Text>(textEntity);
    glm::vec4 color = ecs::entityHasComponent<Color>(textEntity) ? ecs::get<Color>(textEntity).color : glm::vec4{0.5, 0.5, 0.5, 1};
    glm::mat4 matrix = text.matrix.value_or(glm::mat4{1.0f});
    text.font->drawText(text.text, text.position, text.size, color, matrix);
}

void game::Renderer::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{ // good luck reading it
    for(ecs::Entity_t const &cameraEntity : entities) {
        if(!ecs::entityHasComponent<Camera>(cameraEntity) || !ecs::entityHasComponent<RenderTarget>(cameraEntity)) continue;
        Camera &camera = ecs::get<Camera>(cameraEntity);
        RenderTarget &rtarget = ecs::get<RenderTarget>(cameraEntity);

        if(rtarget.prevWidth != camera.width || rtarget.prevHeight != camera.height) { // resize or initialize buffers / textures
        }
        if(rtarget.prevWidth == -1) { // initialize render target
        }
        rtarget.prevWidth = camera.width;
        rtarget.prevHeight = camera.height;
        
        camera.projMat = getProjMat(cameraEntity);
        camera.viewMat = getViewMat(cameraEntity);
        glm::vec3 cameraPosition = glm::vec4{0, 0, 0, 1} * camera.viewMat;
        
        glViewport(0, 0, camera.width, camera.height);
        glBindFramebuffer(GL_FRAMEBUFFER, rtarget.mainFBOid);
        glClearColor(rtarget.clearColor.r, rtarget.clearColor.g, rtarget.clearColor.b, rtarget.clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for(ecs::Entity_t const &entity : entities) {
            if(ecs::entityHasComponent<model::Model>(entity)) {
                model::Model const &model = ecs::get<model::Model>(entity);
                assert(model.isLoaded());
                glm::mat4 modelMat = getModelMat(entity);
                std::vector<glm::mat4> const &boneMatrices = ecs::entityHasComponent<Animation>(entity) && ecs::get<Animation>(entity).boneMatrices != nullptr ?
                    *ecs::get<Animation>(entity).boneMatrices :
                    std::vector<glm::mat4>{0};
                opengl::ShaderProgram &shader = ecs::entityHasComponent<opengl::ShaderProgram>(entity) ? ecs::get<opengl::ShaderProgram>(entity) : m_defaultShader;
                for(auto const &mesh : model.getMeshes()) {
                    if(!mesh.drawable.has_value()) continue;

                    Drawable const &drawable = mesh.drawable.value();
    
                    opengl::Texture const *diffuseTexture = nullptr;
                    for(auto const &texture : mesh.textures) {
                        if(texture.type == "diffuse") diffuseTexture = &texture;
                    }
                    if(!diffuseTexture) diffuseTexture = &m_notfound;

                    shader.bind();
                    diffuseTexture->bind(0);
                    ecs::entityHasComponent<Color>(entity) ?
                        glUniform4fv(shader.getUniform("u_color"), 1, &ecs::get<Color>(entity).color.r) :
                        glUniform4f( shader.getUniform("u_color"), 1, 1, 1, 1);
                    if(boneMatrices.size() != 0) {
                        glUniformMatrix4fv(shader.getUniform("u_boneMatrices"), boneMatrices.size(), GL_FALSE, &(*boneMatrices.data())[0][0]);
                    }
                    glUniform1i(       shader.getUniform("u_texture"),        0);
                    glUniformMatrix4fv(shader.getUniform("u_modelMat"),       1, GL_FALSE, &modelMat[0][0]);
                    glUniformMatrix4fv(shader.getUniform("u_viewMat"),        1, GL_FALSE, &camera.viewMat[0][0]);
                    glUniformMatrix4fv(shader.getUniform("u_projectionMat"),  1, GL_FALSE, &camera.projMat[0][0]);
                    glUniform3fv(      shader.getUniform("u_cameraPosition"), 1, &cameraPosition.x);
                    glEnable(GL_DEPTH_TEST);
                    glEnable(GL_CULL_FACE);
                    draw(drawable);
                }
            }
            if(ecs::entityHasComponent<Text>(entity)) drawText(entity);
        } // for(auto &entity : entities) 
    } // for(auto &cameraEntity : entities)
}
