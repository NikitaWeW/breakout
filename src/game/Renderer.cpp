#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer.hpp"
#include "Animator.hpp"
#include "game/Physics.hpp"
#include "utils/Model.hpp"


glm::mat4 getProjMat(ecs::Entity_t const &entity) 
{
    game::Camera const &camera = ecs::get<game::Camera>(entity);
    assert(camera.width > 0 && camera.height > 0);
    if(ecs::entityHasComponent<game::PerspectiveProjection>(entity))
        return glm::perspective<float>(glm::radians(camera.fov), (float) camera.width / (float) camera.height, camera.znear, camera.zfar);
    else
        return glm::ortho<float>((float) -camera.width / (float) camera.height, (float) camera.width / (float) camera.height, -1, 1, camera.znear, camera.zfar); // FIXME: smashed random shit here
}
glm::mat4 getViewMat(ecs::Entity_t const &entity) 
{
    using namespace game;
    if(ecs::entityHasComponent<OrientationQuaternion>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<Position>(entity) ? ecs::get<Position>(entity).position : glm::vec3{0, 0, 0};
        return glm::translate(glm::mat4_cast(glm::normalize(ecs::get<OrientationQuaternion>(entity).quat)), -position);
    } else if(ecs::entityHasComponent<OrientationEuler>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<Position>(entity) ? ecs::get<Position>(entity).position : glm::vec3{0, 0, 0};
        glm::vec3 orientation = glm::radians(ecs::get<OrientationEuler>(entity).rotation);
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
    if(ecs::entityHasComponent<game::OrientationEuler>(entity)) {
        glm::vec3 const &rotation = ecs::get<game::OrientationEuler>(entity).rotation;
        modelMat = glm::rotate<float>(modelMat, rotation.x, {1, 0, 0});
        modelMat = glm::rotate<float>(modelMat, rotation.y, {0, 1, 0});
        modelMat = glm::rotate<float>(modelMat, rotation.z, {0, 0, 1});
    } else if(ecs::entityHasComponent<game::OrientationQuaternion>(entity)) {
        modelMat = modelMat * glm::mat4_cast(ecs::get<game::OrientationQuaternion>(entity).quat);
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
void drawText(ecs::Entity_t const &textEntity, game::Camera const &camera) {
    using namespace game;
    assert(ecs::entityHasComponent<Text>(textEntity));
    Text const &text = ecs::get<Text>(textEntity);
    glm::mat4 matrix = text.matrix.value_or(glm::ortho<float>(
        (float) -camera.width / (float) camera.height, 
        (float) camera.width / (float) camera.height, 
        -1, 1, camera.znear, camera.zfar
    ));
    text.font->drawText(text.text, text.position, text.size, text.fgColor, text.bgColor, matrix);
}
std::optional<std::vector<glm::mat4> const *> getBoneMatrices(ecs::Entity_t const &entity)
{
    std::optional<std::vector<glm::mat4> const *> boneMatrices = {};
    if(ecs::entityHasComponent<game::Animation>(entity) && ecs::get<game::Animation>(entity).boneMatrices != nullptr) {
        boneMatrices.emplace(ecs::get<game::Animation>(entity).boneMatrices);
    }

    return boneMatrices;
}
void setDefaultTexture(std::string const &type, std::set<std::string> const &boundTextureTypes, std::map<std::string, opengl::Texture> const &defaultTextures, size_t &textureCounter, opengl::ShaderProgram const &shader)
{
    if(boundTextureTypes.find(type) == boundTextureTypes.end()) {
        glUniform1i(shader.getUniform("u_material." + type), static_cast<int>(textureCounter));
        opengl::Texture const *texture = defaultTextures.find(type) != defaultTextures.end() ? 
            &defaultTextures.at(type) : 
            &defaultTextures.at("");

        assert(texture);
        texture->bind(static_cast<unsigned>(textureCounter));
        ++textureCounter;
    }
}
void setTextures(model::Mesh const &mesh, opengl::ShaderProgram const &shader, std::map<std::string, opengl::Texture> const &defaultTextures)
{
    size_t textureCount = 0;
    std::set<std::string> boundTextureTypes;
    for(auto const &texture : mesh.textures) {
        glUniform1i(shader.getUniform("u_material." + texture.type), static_cast<int>(textureCount));
        texture.bind(static_cast<unsigned>(textureCount));
        boundTextureTypes.insert(texture.type);
        ++textureCount;
    }
    setDefaultTexture("diffuse",  boundTextureTypes, defaultTextures, textureCount, shader);
    setDefaultTexture("normal",   boundTextureTypes, defaultTextures, textureCount, shader);
    setDefaultTexture("rough",    boundTextureTypes, defaultTextures, textureCount, shader);
    setDefaultTexture("specular", boundTextureTypes, defaultTextures, textureCount, shader);
    setDefaultTexture("AO",       boundTextureTypes, defaultTextures, textureCount, shader);
    setDefaultTexture("height",   boundTextureTypes, defaultTextures, textureCount, shader);
}
void game::Renderer::drawModel(ecs::Entity_t const &entity, opengl::ShaderProgram const &shader) const
{
    assert(ecs::entityHasComponent<model::Model>(entity));
    model::Model const &model = ecs::get<model::Model>(entity);
    glm::mat4 modelMat = getModelMat(entity);
    std::optional<std::vector<glm::mat4> const *> boneMatrices = getBoneMatrices(entity);
    
    for(auto const &mesh : model.getMeshes()) {
        if(!mesh.drawable.has_value()) continue;

        game::Drawable const &drawable = mesh.drawable.value();
        
        setTextures(mesh, shader, m_defaultTextures);

        if(m_lightsUBO.has_value()) {
            int location = shader.getUniformBlock("u_lights");
            if(location >= 0) {
                m_lightsUBO.value()->bind();
                glUniformBlockBinding(shader.getRenderID(), location, 0);
            }
        }
        
        ecs::entityHasComponent<game::Color>(entity) ?
            glUniform4fv(shader.getUniform("u_color"), 1, &ecs::get<game::Color>(entity).color.r) :
            glUniform4f( shader.getUniform("u_color"), 1, 1, 1, 1);
        if(boneMatrices.has_value()) {
            glUniformMatrix4fv(shader.getUniform("u_boneMatrices"), static_cast<int>(boneMatrices.value()->size()), GL_FALSE, &(*boneMatrices.value()->data())[0][0]);
        }
        if(ecs::entityHasComponent<game::RepeatTexture>(entity)) {
            glUniform1ui(shader.getUniform("u_texCoordMult"), ecs::get<game::RepeatTexture>(entity).num);
        } else {
            glUniform1ui(shader.getUniform("u_texCoordMult"), 1);
        }
        if(ecs::entityHasComponent<game::MaterialProperties>(entity)) {
            game::MaterialProperties const &materialProperties = ecs::get<game::MaterialProperties>(entity);
            glUniform1f(shader.getUniform("u_material.shininess"), materialProperties.shininess);
        }
        glUniform1i(       shader.getUniform("u_animated"),       boneMatrices.has_value());
        glUniformMatrix4fv(shader.getUniform("u_modelMat"),       1, GL_FALSE, &modelMat[0][0]);
        glUniformMatrix4fv(shader.getUniform("u_normalMat"),      1, GL_FALSE, &glm::transpose(glm::inverse(modelMat))[0][0]);
        draw(drawable);
    }
}

void game::Renderer::renderMain(std::set<ecs::Entity_t> const &entities, double deltatime, game::Camera &camera, game::RenderTarget &rtarget)
{
    glViewport(0, 0, camera.width, camera.height);
    
    glm::vec3 cameraPosition = glm::vec3{glm::inverse(camera.viewMat) * glm::vec4{0, 0, 0, 1}};
    {
        auto lightsUBOEntity = std::find_if(entities.begin(), entities.end(), [](ecs::Entity_t const &entity){ return ecs::entityHasComponent<LightUBO>(entity); });
        m_lightsUBO = lightsUBOEntity != entities.end() ? &ecs::get<LightUBO>(*lightsUBOEntity).ubo : std::optional<opengl::UniformBuffer *>{};
    }

    // ===================
    // SOLID OBJECTS PASS 
    // ===================

    // set up render states
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);

    rtarget.mainFBO.bind();
    glClearColor(rtarget.clearColor.r, rtarget.clearColor.g, rtarget.clearColor.b, rtarget.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw opaque objects
    m_propShader.bind();
    glUniformMatrix4fv(m_propShader.getUniform("u_viewMat"),        1, GL_FALSE, &camera.viewMat[0][0]);
    glUniformMatrix4fv(m_propShader.getUniform("u_projectionMat"),  1, GL_FALSE, &camera.projMat[0][0]);
    glUniform3fv(      m_propShader.getUniform("u_camPos"), 1, &cameraPosition.x);
    for(ecs::Entity_t const &entity : entities) {
        if(ecs::entityHasComponent<model::Model>(entity) && (!ecs::entityHasComponent<Transparent>(entity) || ecs::entityHasComponent<SemiTransparent>(entity))) {
            drawModel(entity, m_propShader);
        }
    } // for(auto &entity : entities) 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    // =====================
    // OIT TRANSPARENT PASS 
    // =====================

    // configure render states
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunci(0, GL_ONE, GL_ONE); // accumulation
    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // revelage
    glBlendEquation(GL_FUNC_ADD);

    rtarget.oitFBO.bind();
    {
        constexpr glm::vec4 zeroFiller{0};
        constexpr glm::vec4 oneFiller{1};
        glClearBufferfv(GL_COLOR, 0, &zeroFiller.r);
        glClearBufferfv(GL_COLOR, 1, &oneFiller.r);
    }

    // draw transparent objects
    m_oitShader.bind();
    glUniformMatrix4fv(m_oitShader.getUniform("u_viewMat"),        1, GL_FALSE, &camera.viewMat[0][0]);
    glUniformMatrix4fv(m_oitShader.getUniform("u_projectionMat"),  1, GL_FALSE, &camera.projMat[0][0]);
    glUniform3fv(      m_oitShader.getUniform("u_camPos"), 1, &cameraPosition.x);
    for(ecs::Entity_t const &entity : entities) {
        if(ecs::entityHasComponent<model::Model>(entity) && (ecs::entityHasComponent<Transparent>(entity) || ecs::entityHasComponent<SemiTransparent>(entity))) {
            drawModel(entity, m_oitShader);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    // ===================
    // OIT COMPOSITE PASS 
    // ===================

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rtarget.mainFBO.bind();
    m_oitCompositeShader.bind();
    rtarget.oitAccumTexture.bind(0);
    rtarget.oitRevelageTexture.bind(1);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    // ======================================
    // HDR IMAGE / OTHER POSTPROCESSING PASS 
    // ======================================

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, rtarget.outputFBOid);
    m_screenShader.bind();
    rtarget.mainFBOColor.bind(0);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void game::Renderer::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &cameraEntity : entities) {
        if(!ecs::entityHasComponent<Camera>(cameraEntity) || !ecs::entityHasComponent<RenderTarget>(cameraEntity)) continue;

        game::Camera &camera = ecs::get<game::Camera>(cameraEntity);
        game::RenderTarget &rtarget = ecs::get<game::RenderTarget>(cameraEntity);
        if(rtarget.prevWidth != camera.width || rtarget.prevHeight != camera.height) { // resize or initialize buffers / textures
            rtarget.oitAccumTexture.bind();     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, camera.width, camera.height, 0, GL_RGBA, GL_FLOAT, nullptr);
            rtarget.oitRevelageTexture.bind();  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, camera.width, camera.height, 0, GL_RED, GL_FLOAT, nullptr);

            rtarget.mainFBOColor.bind();        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, camera.width, camera.height, 0, GL_RGBA, GL_FLOAT, nullptr);
            rtarget.mainFBORBO.bind();          glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, camera.width, camera.height);
        }
        if(rtarget.prevWidth == -1) { // initialize render target
            rtarget.oitFBO.bind();
            rtarget.oitFBO.attach(rtarget.oitAccumTexture, GL_COLOR_ATTACHMENT0);
            rtarget.oitFBO.attach(rtarget.oitRevelageTexture, GL_COLOR_ATTACHMENT1);
            rtarget.oitFBO.attach(rtarget.mainFBORBO, GL_DEPTH_STENCIL_ATTACHMENT);
            {
                GLenum const drawbuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                glDrawBuffers(sizeof(drawbuffers) / sizeof(*drawbuffers), drawbuffers);
            }
            assert(rtarget.oitFBO.isComplete());

            rtarget.mainFBO.bind();
            rtarget.mainFBO.attach(rtarget.mainFBOColor, GL_COLOR_ATTACHMENT0);
            rtarget.mainFBO.attach(rtarget.mainFBORBO, GL_DEPTH_STENCIL_ATTACHMENT);
            assert(rtarget.mainFBO.isComplete());
        }
        rtarget.prevWidth = camera.width;
        rtarget.prevHeight = camera.height;
        
        camera.projMat = getProjMat(cameraEntity);
        camera.viewMat = getViewMat(cameraEntity);

        renderMain(entities, deltatime, camera, rtarget);

        for(ecs::Entity_t const &entity : entities) {
            if(ecs::entityHasComponent<game::Text>(entity)) drawText(entity, camera);
        } // for(auto &entity : entities) 
    } // for(auto &cameraEntity : entities)
}

void game::LightUpdater::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &storageEntity : entities) {
        if(!ecs::entityHasComponent<LightStorage>(storageEntity) || !ecs::entityHasComponent<LightUBO>(storageEntity)) continue;
        LightStorage &storage = ecs::get<LightStorage>(storageEntity);
        opengl::UniformBuffer &ubo = ecs::get<LightUBO>(storageEntity).ubo;
        if(ubo.getRenderID() == 0) {
            ubo = opengl::UniformBuffer{0}; // dummy argument
            ubo.bindingPoint(0);
        }

        storage.numPointLights = 0;
        storage.numDirLights = 0;
        storage.numSpotLights = 0;
        for(ecs::Entity_t const &lightEntity : entities) {
            if(!ecs::entityHasComponent<Light>(lightEntity)) continue;
            Light const &light = ecs::get<Light>(lightEntity);

            if(ecs::entityHasComponent<PointLight>(lightEntity)) {
                ShaderPointLight &shaderPointLight = storage.pointLights[storage.numPointLights];
                PointLight const &pointLight = ecs::get<PointLight>(lightEntity);

                shaderPointLight.attenuation = pointLight.attenuation;
                shaderPointLight.color = light.color;
                shaderPointLight.position = ecs::entityHasComponent<Position>(lightEntity) ?
                    ecs::get<Position>(lightEntity).position :
                    glm::vec3{0};
                ++storage.numPointLights;
            } else if(ecs::entityHasComponent<DirectionalLight>(lightEntity)) {
                ShaderDirLight &shaderDirLight = storage.dirLights[storage.numDirLights];

                shaderDirLight.direction = ecs::entityHasComponent<Direction>(lightEntity) ?
                    ecs::get<Direction>(lightEntity).dir :
                    glm::vec3{0, 0, -1};
                shaderDirLight.color = light.color;
                ++storage.numDirLights;
            } else if(ecs::entityHasComponent<SpotLight>(lightEntity)) {
                ShaderSpotLight &shaderSpotLight = storage.spotLights[storage.numSpotLights];
                SpotLight const &spotLight = ecs::get<SpotLight>(lightEntity);

                shaderSpotLight = {
                    .position = ecs::entityHasComponent<Position>(lightEntity) ?
                        ecs::get<Position>(lightEntity).position :
                        glm::vec3{0, 0, -1},
                    .innerConeAngle = glm::cos(glm::radians(spotLight.innerConeAngle)),
                    .direction = ecs::entityHasComponent<Direction>(lightEntity) ?
                        ecs::get<Direction>(lightEntity).dir :
                        glm::vec3{0, 0, -1},
                    .outerConeAngle = glm::cos(glm::radians(spotLight.outerConeAngle)),
                    .attenuation = spotLight.attenuation,
                    .color = light.color,
                };
                shaderSpotLight.color = light.color;
                ++storage.numSpotLights;
            }
        }
        
        ubo.bind();
        glBufferData(GL_UNIFORM_BUFFER, sizeof(storage), &storage, GL_DYNAMIC_DRAW);
    }
}
