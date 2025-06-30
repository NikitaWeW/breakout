#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer.hpp"
#include "Animator.hpp"
#include "game/Physics.hpp"
#include "utils/Model.hpp"
#include "utils/Profiler.hpp"

glm::mat4 getProjMat(ecs::Entity_t const &entity) 
{
    game::Camera const &camera = ecs::get<game::Camera>(entity);
    assert(camera.width > 0 && camera.height > 0);
    if(ecs::entityHasComponent<game::PerspectiveProjection>(entity))
        return glm::perspective<float>(glm::radians(camera.fov), (float) camera.width / (float) camera.height, camera.znear, camera.zfar);
    else
        return glm::ortho<float>(
            (float) -camera.width / 2, 
            (float) camera.width / 2, 
            (float) -camera.height / 2, 
            (float) camera.height / 2, 
            camera.znear, 
            camera.zfar
        );
}
glm::mat4 getViewMat(ecs::Entity_t const &entity) 
{
    using namespace game;
    if(ecs::entityHasComponent<OrientationQuaternion>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<Position>(entity) ? ecs::get<Position>(entity).position : glm::vec3{0, 0, 0};
        return glm::translate(glm::mat4_cast(glm::normalize(ecs::get<OrientationQuaternion>(entity).quat)), -position);
    } else if(ecs::entityHasComponent<Direction>(entity)) {
        glm::vec3 position = ecs::entityHasComponent<game::Position>(entity) ? ecs::get<game::Position>(entity).position : glm::vec3{0};
        glm::vec3 direction = glm::normalize(ecs::get<game::Direction>(entity).dir);
        assert(direction != glm::vec3{0});
        glm::vec3 up = glm::mix(glm::vec3{0, 1, 0}, glm::vec3{1, 0, 0}, glm::abs(glm::dot(direction, glm::vec3{0, 1, 0})));
        return glm::lookAt(position, position + direction, up);
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
void setTextures(model::Mesh const &mesh, opengl::ShaderProgram const &shader, std::map<std::string, opengl::Texture> const &defaultTextures, unsigned const &commonTextureCount)
{
    size_t textureCount = commonTextureCount;
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
void setCommonUniforms(opengl::ShaderProgram const &shader, game::Camera const &camera, glm::vec3 const &cameraPosition, unsigned &commonTextureCount, std::optional<opengl::UniformBuffer *> const &lightsUBO, std::optional<game::LightSamplers *> const &lightSamplers)
{
    glUniformMatrix4fv(shader.getUniform("u_viewMat"),        1, GL_FALSE, &camera.viewMat[0][0]);
    glUniformMatrix4fv(shader.getUniform("u_projectionMat"),  1, GL_FALSE, &camera.projMat[0][0]);
    glUniform3fv(      shader.getUniform("u_camPos"),         1,           &cameraPosition.x);
    if(lightsUBO.has_value()) {
        int location = shader.getUniformBlock("u_lights");
        if(location >= 0) {
            lightsUBO.value()->bind();
            glUniformBlockBinding(shader.getRenderID(), location, 0);
        }
    }
    if(lightSamplers.has_value()) {
        auto &samplers = *lightSamplers.value();
        for(auto &[index, texture] : samplers.pointLightSamplers) {
            glUniform1i(shader.getUniform("u_pointLightSamplers[" + std::to_string(index) + "]"), commonTextureCount);
            texture->bind(commonTextureCount);
            ++commonTextureCount;
        }
        for(auto &[index, texture] : samplers.dirLightSamplers) {
            glUniform1i(shader.getUniform("u_dirLightSamplers[" + std::to_string(index) + "]"), commonTextureCount);
            texture->bind(commonTextureCount);
            ++commonTextureCount;
        }
        for(auto &[index, texture] : samplers.spotLightSamplers) {
            glUniform1i(shader.getUniform("u_spotLightSamplers[" + std::to_string(index) + "]"), commonTextureCount);
            texture->bind(commonTextureCount);
            ++commonTextureCount;
        }
    }

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
    PROFILER_PROFILE();
    using namespace game;
    assert(ecs::entityHasComponent<Text>(textEntity));
    Text const &text = ecs::get<Text>(textEntity);
    glm::mat4 matrix = text.matrix.value_or(glm::ortho<float>(
        0, static_cast<float>(camera.width), 
        0, static_cast<float>(camera.height),
        -1, 1
    ));
    text.font->drawText(text.text, text.position * glm::vec2{camera.width, camera.height}, text.size * static_cast<float>(camera.height), text.fgColor, text.bgColor, matrix);
}
void game::Renderer::drawModel(ecs::Entity_t const &entity, opengl::ShaderProgram const &shader) const
{
    PROFILER_PROFILE();
    assert(ecs::entityHasComponent<model::Model>(entity));
    model::Model const &model = ecs::get<model::Model>(entity);
    glm::mat4 modelMat = getModelMat(entity);
    std::optional<std::vector<glm::mat4> const *> boneMatrices = getBoneMatrices(entity);
    
    for(auto const &mesh : model.getMeshes()) {
        if(!mesh.drawable.has_value()) continue;

        game::Drawable const &drawable = mesh.drawable.value();
        
        setTextures(mesh, shader, m_defaultTextures, m_commonTextureCount);
        
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
        glUniform1i(       shader.getUniform("u_animated"),  boneMatrices.has_value());
        glUniformMatrix4fv(shader.getUniform("u_modelMat"),  1, GL_FALSE, &modelMat[0][0]);
        glUniformMatrix4fv(shader.getUniform("u_normalMat"), 1, GL_FALSE, &glm::transpose(glm::inverse(modelMat))[0][0]);
        draw(drawable);
    }
}
void drawSkybox(ecs::Entity_t const &entity, opengl::Cubemap const &cubemap) 
{
    PROFILER_PROFILE();
    cubemap.bind(0);

    // draw a cube (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

void setupPointLightShadowCaster(ecs::Entity_t const &entity, game::ShadowCaster &shadowCaster, game::Camera const &camera)
{
    if(!shadowCaster.omnidirectionalShadowMap.has_value()) {
        shadowCaster.omnidirectionalShadowMap = opengl::Cubemap{0}; // dummy argument
        shadowCaster.omnidirectionalShadowMap.value().bind();
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        for(unsigned int i = 0; i < 6; ++i) 
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, game::SHADOW_MAP_SIZE, game::SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        assert(shadowCaster.omnidirectionalShadowMap.has_value() && shadowCaster.omnidirectionalShadowMap.value().getRenderID() != 0);

        shadowCaster.fbo = opengl::Framebuffer{0};
        shadowCaster.fbo.bind();
        shadowCaster.fbo.attach(shadowCaster.omnidirectionalShadowMap.value(), GL_DEPTH_ATTACHMENT);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        assert(shadowCaster.fbo.isComplete());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        shadowCaster.projMat = glm::perspective<float>(glm::radians(90.0f), 1, game::SHADOW_MAP_ZNEAR, game::SHADOW_MAP_ZFAR);
        shadowCaster.nearPlane = game::SHADOW_MAP_ZNEAR;
        shadowCaster.farPlane = game::SHADOW_MAP_ZFAR;
    }
    shadowCaster.viewMat = getViewMat(entity);
}
void setupDirLightShadowCaster(ecs::Entity_t const &entity, game::ShadowCaster &shadowCaster, game::Camera const &camera) 
{
    if(!shadowCaster.regularShadowMap.has_value()) {
        shadowCaster.regularShadowMap = opengl::Texture{GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE}; // dummy argument
        shadowCaster.regularShadowMap.value().bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, game::SHADOW_MAP_SIZE, game::SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        assert(shadowCaster.regularShadowMap.has_value() && shadowCaster.regularShadowMap.value().getRenderID() != 0);

        shadowCaster.fbo = opengl::Framebuffer{0};
        shadowCaster.fbo.bind();
        shadowCaster.fbo.attach(shadowCaster.regularShadowMap.value(), GL_DEPTH_ATTACHMENT);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        assert(shadowCaster.fbo.isComplete());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        constexpr float range = 25;
        shadowCaster.projMat = glm::ortho(-range, range, -range, range, game::SHADOW_MAP_ZNEAR, game::SHADOW_MAP_ZFAR);
        shadowCaster.nearPlane = game::SHADOW_MAP_ZNEAR;
        shadowCaster.farPlane = game::SHADOW_MAP_ZFAR;
    }
    glm::vec3 direction = glm::vec3{0, -1, 0};
    if(ecs::entityHasComponent<game::Direction>(entity))
        direction = ecs::get<game::Direction>(entity).dir;
        
    // glm::vec3 targetPosition = glm::vec3{glm::inverse(camera.viewMat) * glm::vec4{0, 0, 0, 1}};
    glm::vec3 targetPosition = glm::vec3{0};
    shadowCaster.viewMat = glm::lookAt(targetPosition - 30.0f * glm::normalize(direction), targetPosition, glm::vec3{0, 1, 0}); // FIXME
}
void setupSpotLightShadowCaster(ecs::Entity_t const &entity, game::ShadowCaster &shadowCaster, game::Camera const &camera) 
{
    auto const &light = ecs::get<game::SpotLight>(entity);
    if(!shadowCaster.regularShadowMap.has_value()) {
        shadowCaster.regularShadowMap = opengl::Texture{GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE}; // dummy argument
        shadowCaster.regularShadowMap.value().bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, game::SHADOW_MAP_SIZE, game::SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        assert(shadowCaster.regularShadowMap.has_value() && shadowCaster.regularShadowMap.value().getRenderID() != 0);

        shadowCaster.fbo = opengl::Framebuffer{0}; // also a dummy argument
        shadowCaster.fbo.bind();
        shadowCaster.fbo.attach(shadowCaster.regularShadowMap.value(), GL_DEPTH_ATTACHMENT);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        assert(shadowCaster.fbo.isComplete());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        shadowCaster.projMat = glm::perspective<float>(glm::radians(light.outerConeAngle), 1, game::SHADOW_MAP_ZNEAR, game::SHADOW_MAP_ZFAR);
        shadowCaster.nearPlane = game::SHADOW_MAP_ZNEAR;
        shadowCaster.farPlane = game::SHADOW_MAP_ZFAR;
    }
    shadowCaster.viewMat = getViewMat(entity);
}
void setupAreaLightShadowCaster(ecs::Entity_t const &entity, game::ShadowCaster &shadowCaster, game::Camera const &camera)
{
    assert(false && "not implemented"); // TODO: area lights
}

void game::Renderer::renderMain(std::set<ecs::Entity_t> const &entities, game::Camera &camera, game::RenderTarget &rtarget)
{
    PROFILER_PROFILE();
    glViewport(0, 0, camera.width, camera.height);
    
    glm::mat4 invViewMat = glm::inverse(camera.viewMat);
    glm::vec3 cameraPosition = glm::vec3{invViewMat * glm::vec4{0, 0, 0, 1}};

    {
        auto lightStorageEntity = std::find_if(entities.begin(), entities.end(), [](ecs::Entity_t const &entity){ return ecs::entityHasComponent<LightUBO>(entity); });
        m_lightsUBO = lightStorageEntity != entities.end() ? &ecs::get<LightUBO>(*lightStorageEntity).ubo : std::optional<opengl::UniformBuffer *>{};
        m_lightSamplers = lightStorageEntity != entities.end() ? &ecs::get<LightSamplers>(*lightStorageEntity) : std::optional<LightSamplers *>{};
    }

    // ===================
    // SOLID OBJECTS PASS 
    // ===================

    // set up render states
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    rtarget.mainFBO.bind();
    glClearColor(rtarget.clearColor.r, rtarget.clearColor.g, rtarget.clearColor.b, rtarget.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw opaque objects
    m_propShader.bind();
    setCommonUniforms(m_propShader, camera, cameraPosition, m_commonTextureCount, m_lightsUBO, m_lightSamplers);
    for(ecs::Entity_t const &entity : entities) {
        if(ecs::entityHasComponent<model::Model>(entity) && (!ecs::entityHasComponent<Transparent>(entity) || ecs::entityHasComponent<SemiTransparent>(entity))) {
            drawModel(entity, m_propShader);
        }
    } // for(auto &entity : entities) 

    // draw skybox
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    m_skyboxShader.bind();
    glUniformMatrix4fv(m_skyboxShader.getUniform("u_viewMat"),        1, GL_FALSE, &camera.viewMat[0][0]);
    glUniformMatrix4fv(m_skyboxShader.getUniform("u_projectionMat"),  1, GL_FALSE, &camera.projMat[0][0]);
    for(ecs::Entity_t const &entity : entities) {
        if(ecs::entityHasComponent<Skybox>(entity) && ecs::entityHasComponent<opengl::Cubemap>(entity)) {
            drawSkybox(entity, ecs::get<opengl::Cubemap>(entity));
        }
    } // for(auto &entity : entities) 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    // =====================
    // OIT TRANSPARENT PASS 
    // =====================

    // configure render states
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunci(0, GL_ONE, GL_ONE); // accumulation
    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // revelage
    glBlendEquation(GL_FUNC_ADD);
    glCullFace(GL_BACK);

    rtarget.oitFBO.bind();
    {
        constexpr glm::vec4 zeroFiller{0};
        constexpr glm::vec4 oneFiller{1};
        glClearBufferfv(GL_COLOR, 0, &zeroFiller.r);
        glClearBufferfv(GL_COLOR, 1, &oneFiller.r);
    }

    // draw transparent objects
    m_oitShader.bind();
    setCommonUniforms(m_oitShader, camera, cameraPosition, m_commonTextureCount, m_lightsUBO, m_lightSamplers);
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
    glCullFace(GL_BACK);
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
    glCullFace(GL_BACK);
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
void game::Renderer::renderShadowMaps(std::set<ecs::Entity_t> const &entities, game::Camera &camera, game::RenderTarget &rtarget) {
    PROFILER_PROFILE();
    for(ecs::Entity_t const &lightEntity : entities) {
        if(!ecs::entityHasComponent<Light>(lightEntity) || !ecs::entityHasComponent<ShadowCaster>(lightEntity)) continue;

        ShadowCaster &shadowCaster = ecs::get<ShadowCaster>(lightEntity);

        opengl::ShaderProgram *opaqueShader = nullptr;

        if(ecs::entityHasComponent<PointLight>(lightEntity)) {
            opaqueShader = &m_depthMapOmnidirectionalShader;
            setupPointLightShadowCaster(lightEntity, shadowCaster, camera);
        }
        else if(ecs::entityHasComponent<DirectionalLight>(lightEntity)) {
            opaqueShader = &m_depthMapShader;
            setupDirLightShadowCaster(lightEntity, shadowCaster, camera);
        }
        else if(ecs::entityHasComponent<SpotLight>(lightEntity)) {
            opaqueShader = &m_depthMapShader;
            setupSpotLightShadowCaster(lightEntity, shadowCaster, camera);
        }
        else if(ecs::entityHasComponent<AreaLight>(lightEntity)) {
            opaqueShader = &m_depthMapShader;
            setupAreaLightShadowCaster(lightEntity, shadowCaster, camera);
        }
        
        assert(opaqueShader);
        
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

        // set up render states
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shadowCaster.fbo.bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        opaqueShader->bind();
        glUniformMatrix4fv(opaqueShader->getUniform("u_viewMat"),        1, GL_FALSE, &shadowCaster.viewMat[0][0]);
        glUniformMatrix4fv(opaqueShader->getUniform("u_projectionMat"),  1, GL_FALSE, &shadowCaster.projMat[0][0]);
        for(ecs::Entity_t const &entity : entities) {
            if(ecs::entityHasComponent<model::Model>(entity) && ecs::entityHasComponent<CastsShadow>(entity)) {
                drawModel(entity, *opaqueShader);
            }
        } // for(auto &entity : entities) 
    }
}

void game::Renderer::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    PROFILER_PROFILE();
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
                std::array<GLenum, 2> const drawbuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                glDrawBuffers(drawbuffers.size(), drawbuffers.data());
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

        m_commonTextureCount = 0;

        renderShadowMaps(entities, camera, rtarget);
        renderMain(entities, camera, rtarget);

        for(ecs::Entity_t const &entity : entities) {
            if(ecs::entityHasComponent<game::Text>(entity)) drawText(entity, camera);
        } // for(auto &entity : entities) 
    } // for(auto &cameraEntity : entities)
}

game::LightUpdater::LightUpdater()
{
    ecs::Entity_t lightStorageEntity = ecs::makeEntity<LightUBO, LightStorage, LightSamplers>();
    ecs::get<LightUBO>(lightStorageEntity) = {};
    ecs::get<LightUpdater::LightStorage>(lightStorageEntity) = {};
    ecs::getSystemManager().getEntities().insert(lightStorageEntity);
}
void game::LightUpdater::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    PROFILER_PROFILE();
    for(ecs::Entity_t const &storageEntity : entities) {
        if(!ecs::entityHasComponent<LightStorage>(storageEntity) || !ecs::entityHasComponent<LightUBO>(storageEntity) || !ecs::entityHasComponent<LightSamplers>(storageEntity)) continue;
        LightStorage &storage = ecs::get<LightStorage>(storageEntity);
        opengl::UniformBuffer &ubo = ecs::get<LightUBO>(storageEntity).ubo;
        LightSamplers &samplers = ecs::get<LightSamplers>(storageEntity);
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
            std::optional<ShadowCaster *> caster = ecs::entityHasComponent<ShadowCaster>(lightEntity) ? &ecs::get<ShadowCaster>(lightEntity) : std::optional<ShadowCaster *>{};
            glm::mat4 viewProj = caster.has_value() ? caster.value()->projMat * caster.value()->viewMat : glm::mat4{1.0f};

            if(ecs::entityHasComponent<PointLight>(lightEntity)) {
                ShaderPointLight &shaderPointLight = storage.pointLights[storage.numPointLights];
                PointLight const &pointLight = ecs::get<PointLight>(lightEntity);

                shaderPointLight.attenuation = pointLight.attenuation;
                shaderPointLight.color = light.color;
                shaderPointLight.position = ecs::entityHasComponent<Position>(lightEntity) ?
                    ecs::get<Position>(lightEntity).position :
                    glm::vec3{0};
                // shaderPointLight.viewProj = viewProj;
                if(caster.has_value()) 
                    shaderPointLight.farPlane = caster.value()->farPlane;

                if(caster.has_value() && caster.value()->omnidirectionalShadowMap.has_value()) 
                    samplers.pointLightSamplers[storage.numPointLights] = &caster.value()->omnidirectionalShadowMap.value();

                ++storage.numPointLights;
            } else if(ecs::entityHasComponent<DirectionalLight>(lightEntity)) {
                ShaderDirLight &shaderDirLight = storage.dirLights[storage.numDirLights];

                shaderDirLight.direction = ecs::entityHasComponent<Direction>(lightEntity) ?
                    ecs::get<Direction>(lightEntity).dir :
                    glm::vec3{0, 0, -1};
                shaderDirLight.color = light.color;
                shaderDirLight.viewProj = viewProj;

                if(caster.has_value() && caster.value()->regularShadowMap.has_value()) 
                    samplers.dirLightSamplers[storage.numDirLights] = &caster.value()->regularShadowMap.value();

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
                shaderSpotLight.viewProj = viewProj;

                if(caster.has_value() && caster.value()->regularShadowMap.has_value()) 
                    samplers.spotLightSamplers[storage.numSpotLights] = &caster.value()->regularShadowMap.value();

                ++storage.numSpotLights;
            } else if(ecs::entityHasComponent<AreaLight>(lightEntity)) {
                assert(false && "not implemented"); // TODO: area lights
            }
        }
        
        ubo.bind();
        glBufferData(GL_UNIFORM_BUFFER, sizeof(storage), &storage, GL_DYNAMIC_DRAW);
    }
}
