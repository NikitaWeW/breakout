#include "engine/Renderer/EngineRenderer.hpp"
#include "../detail.hpp"
#include "engine/Resource/Resources.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace ogl = engine::ogl;

static void bindTexture(int location, ogl::Texture texture, ogl::Texture defaultTexture, unsigned &slot)
{
    if(location == -1 || (texture.id == 0 && defaultTexture.id == 0))
        return;

    if(texture.id)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        glUniform1i(location, slot);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, defaultTexture.id);
        glUniform1i(location, slot);
    }

    ++slot;
}
static void bindTextures(ogl::Program const &program, engine::renderer::Material::Textures const &textures, ogl::Texture defaultTexture)
{
    unsigned slot = 5;
    bindTexture(ogl::getUniform(program, "u_material.textures.albedo"), textures.albedo, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.metallic"), textures.metallic, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.roughness"), textures.roughness, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.normal"), textures.normal, defaultTexture, slot);
}
static void sendMaterial(ogl::Program const &program, engine::Material::Properties const &material)
{
    // glUniform3fv(ogl::getUniform(program, "u_material.properties.ambient"), 1, glm::value_ptr(material.ambient));
    // glUniform3fv(ogl::getUniform(program, "u_material.properties.specular"), 1, glm::value_ptr(material.specular));
    // glUniform3fv(ogl::getUniform(program, "u_material.properties.emission"), 1, glm::value_ptr(material.emission));
    glUniform4fv(ogl::getUniform(program, "u_material.properties.albedo"), 1, glm::value_ptr(material.albedo));

    // glUniform1f(ogl::getUniform(program, "u_material.properties.shininess"), material.shininess);
    // glUniform1f(ogl::getUniform(program, "u_material.properties.metallic"), material.metallic);
    // glUniform1f(ogl::getUniform(program, "u_material.properties.ior"), material.ior);
}

void engine::EngineRendererImpl::renderMainInstance(ogl::Program const &shader, ecs::entity const &e_instance) 
{
    auto const &instance = mReg->get<engine::Instance>(e_instance);
    ENGINE_ASSERT_MSG(mReg->has<renderer::ProcessedTag>(instance.e_model), "Forgot to call engine::EngineRenderer::processData()?");
    renderer::Model const &model = mReg->get<renderer::Model>(instance.e_model);
    bool animated = model.animated && mReg->has<CurrentAnimation>(e_instance);

    for(auto const &mesh : model.meshes)
    {
        glm::mat4 modelMat = mReg->has<engine::Transform>(e_instance) ? mReg->get<engine::Transform>(e_instance).getMat() : glm::mat4{1.0f};
        glm::mat3 normalMat = glm::transpose(glm::inverse(modelMat));
        renderer::Material material = mReg->has<engine::Material>(e_instance) ? renderer::convertMaterial(*mReg, mReg->get<engine::Material>(e_instance)) : mesh.material;

        bindTextures(shader, material.textures, mDefaultTexture);
        sendMaterial(shader, material.properties);

        glUniformMatrix3fv(ogl::getUniform(shader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
        glUniformMatrix4fv(ogl::getUniform(shader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
        if(animated)
        {
            auto const &boneMatrices = mReg->get<CurrentAnimation>(e_instance).boneMatrices;
            ENGINE_ASSERT(boneMatrices.size() == model.skeleton.boneMap.size());
            glUniformMatrix4fv(ogl::getUniform(shader, "u_boneMatrices"), boneMatrices.size(), false, glm::value_ptr(boneMatrices.front())); // TODO: switch to ssbo or ubo.
        }
        glUniform1i (ogl::getUniform(shader, "u_animated"), animated);

        glBindVertexArray(mesh.vao.id);
        glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
    }
}

void engine::EngineRendererImpl::renderMain()
{ 
    // TODO: split render passes and make a system for custom user graphics (render graph?)`
    auto const &camera = mReg->get<engine::Camera>(mConfig.e_camera);

    glViewport(0, 0, camera.size.x, camera.size.y);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mLightManager.getPointLights().id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mLightManager.getDirLights().id  );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mLightManager.getSpotLights().id );
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, mLightManager.getAtlas().texture.id);
    
    // For solid and oit transparent passes
    glUseProgram(mShaders.propShader.id);
    glUniformMatrix4fv(ogl::getUniform(mShaders.propShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
    glUniformMatrix4fv(ogl::getUniform(mShaders.propShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));
    glUniform1ui(      ogl::getUniform(mShaders.propShader, "u_numPointLights"), mLightManager.getNumPointLights());
    glUniform1ui(      ogl::getUniform(mShaders.propShader, "u_numDirLights"),   mLightManager.getNumDirLights());
    glUniform1ui(      ogl::getUniform(mShaders.propShader, "u_numSpotLights"),  mLightManager.getNumSpotLights());
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // ===================
    // SOLID OBJECTS PASS 
    // ===================

    // set up render states
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, mMainFbo.id);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // draw opaque objects
    glUniform1i(ogl::getUniform(mShaders.propShader, "u_transparent"), false);
    for(ecs::entity e_instance : mReg->view<Instance>(ecs::exclude_t<Transparent>{}))
        renderMainInstance(mShaders.propShader, e_instance);

    // =====================
    // OIT TRANSPARENT PASS 
    // =====================

    // configure render states
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunci(1, GL_ONE, GL_ONE); // accumulation
    glBlendFunci(2, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // revealage
    glBlendEquation(GL_FUNC_ADD);
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, mOitFbo.id);
    {
        constexpr glm::vec4 accumulationClearColor{0};
        constexpr glm::vec4 revealageClearColor{1};
        glClearBufferfv(GL_COLOR, 1, &accumulationClearColor.r);
        glClearBufferfv(GL_COLOR, 2, &revealageClearColor.r);
    }

    // draw transparent objects
    glUniform1i(ogl::getUniform(mShaders.propShader, "u_transparent"), true);
    for(ecs::entity e_instance : mReg->view<Instance, Transparent>()) 
        renderMainInstance(mShaders.propShader, e_instance);

    // =====================
    // draw skybox
    // =====================
    {
        auto skyboxView = mReg->view<Skybox>();
        if(!skyboxView.empty())
        {
            auto const &skybox = mReg->get<Skybox>(skyboxView[0]);
            if(skyboxView.size() > 1)
            {
                ENGINE_ASSERT(mReg->has<Cubemap>(skybox.e_cubemap));
                ENGINE_CORE_WARN("Multiple sky boxes found. Drawing the first one: \"{}\"", mReg->get<Cubemap>(skybox.e_cubemap).path);
            }
            ENGINE_ASSERT_MSG(mReg->has<renderer::ProcessedTag>(skybox.e_cubemap), "Forgot to call engine::EngineRenderer::processData()?");
            auto const &cubemap = mReg->get<ogl::Cubemap>(skybox.e_cubemap);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
            glCullFace(GL_BACK);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glDisable(GL_CULL_FACE);
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

            glBindFramebuffer(GL_FRAMEBUFFER, mMainFbo.id);
            glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
            
            glUseProgram(mShaders.skyboxShader.id);
            glUniformMatrix4fv(ogl::getUniform(mShaders.skyboxShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
            glUniformMatrix4fv(ogl::getUniform(mShaders.skyboxShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
        }
    }

    // ===================
    // OIT COMPOSITE PASS 
    // ===================

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, mMainFbo.id);
    glUseProgram(mShaders.oitCompositeShader.id);
    glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, mOitAccumTexture.id);
    glActiveTexture(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, mOitRevealageTexture.id);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ======================================
    // HDR IMAGE / OTHER POSTPROCESSING PASS 
    // ======================================

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(mShaders   .screenShader.id);
    glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, mMainFboColor.id);
    if(mIsDebugView)
        glBindTexture(GL_TEXTURE_2D, mLightManager.getAtlas().texture.id);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
