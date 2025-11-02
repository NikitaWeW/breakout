#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "engine/renderer/engine_renderer/detail/detail.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace ogl = engine::renderer::ogl;

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
static void bindTextures(ogl::Program const &program, engine::renderer::MaterialTextures const &textures, ogl::Texture defaultTexture)
{
    unsigned slot = 0;
    bindTexture(ogl::getUniform(program, "u_material.textures.albedo"), textures.albedo, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.metallic"), textures.metallic, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.roughness"), textures.roughness, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.textures.normal"), textures.normal, defaultTexture, slot);
}
static void sendMaterial(ogl::Program const &program, engine::Material::Properties const &material)
{
    glUniform3fv(ogl::getUniform(program, "u_material.properties.ambient"), 1, glm::value_ptr(material.ambient));
    glUniform3fv(ogl::getUniform(program, "u_material.properties.specular"), 1, glm::value_ptr(material.specular));
    glUniform3fv(ogl::getUniform(program, "u_material.properties.emission"), 1, glm::value_ptr(material.emission));
    glUniform4fv(ogl::getUniform(program, "u_material.properties.albedo"), 1, glm::value_ptr(material.albedo));

    glUniform1f(ogl::getUniform(program, "u_material.properties.shininess"), material.shininess);
    glUniform1f(ogl::getUniform(program, "u_material.properties.metallic"), material.metallic);
    glUniform1f(ogl::getUniform(program, "u_material.properties.ior"), material.ior);
}

void engine::EngineRenderer::renderMainInstance(ecs::registry &reg, ogl::Program const &shader, renderer::RendererData const &data, ecs::entity const &e_instance)
{
    renderer::Model const &model = reg.get<renderer::Model>(reg.get<renderer::ProcessedModel>(reg.get<engine::Instance>(e_instance).e_model).data);
    bool animated = model.animated && reg.has<CurrentAnimation>(e_instance);

    for(auto const &mesh : model.meshes)
    {
        glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_instance) ? reg.get<engine::ModelMatrix>(e_instance) : glm::mat4{1.0f};
        glm::mat4 normalMat = glm::transpose(glm::inverse(modelMat));

        bindTextures(shader, mesh.textures, data.defaultTexture);
        sendMaterial(shader, mesh.material);
        
        glUniformMatrix4fv(ogl::getUniform(shader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
        glUniformMatrix4fv(ogl::getUniform(shader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
        if(animated)
        {
            auto const &boneMatrices = reg.get<CurrentAnimation>(e_instance).boneMatrices;
            ENGINE_ASSERT(boneMatrices.size() == model.skeleton.boneMap.size());
            glUniformMatrix4fv(ogl::getUniform(shader, "u_boneMatrices"), boneMatrices.size(), false, glm::value_ptr(boneMatrices.front())); // TODO: switch to ssbo or ubo.
        }
        glUniform1i (ogl::getUniform(shader, "u_animated"), animated);

        glBindVertexArray(mesh.vao.id);
        glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
    }
}

void engine::EngineRenderer::renderMain(ecs::registry &reg, renderer::RendererData &data)
{
    auto const &camera = reg.get<engine::Camera>(m_context.e_camera);
    glm::mat4 viewMat = reg.has<engine::ModelMatrix>(m_context.e_camera) ? reg.get<engine::ModelMatrix>(m_context.e_camera) : glm::mat4{1.0f};
    
    glViewport(0, 0, camera.size.x, camera.size.y);

    // For solid and oit transparent passes
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, data.lightUBO.id);
    
    glUseProgram(data.propShader.id);
    glUniformBlockBinding(data.propShader.id, ogl::getUniformBlock(data.propShader, "u_lights"), 0);
    glUniformMatrix4fv(ogl::getUniform(data.propShader, "u_viewMat"), 1, false, glm::value_ptr(viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.propShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));

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
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, data.mainFBO.id);
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // draw opaque objects
    glUniform1i(ogl::getUniform(data.propShader, "u_transparent"), false);
    for(ecs::entity e_instance : reg.view<Instance>(ecs::exclude_t<Transparent>{}))
        renderMainInstance(reg, data.propShader, data, e_instance);

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
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, data.oitFBO.id);
    {
        constexpr glm::vec4 zeroFiller{0};
        constexpr glm::vec4 oneFiller{1};
        glClearBufferfv(GL_COLOR, 1, &zeroFiller.r);
        glClearBufferfv(GL_COLOR, 2, &oneFiller.r);
    }

    // draw transparent objects
    glUniform1i(       ogl::getUniform(data.propShader, "u_transparent"), true);
    for(ecs::entity e_instance : reg.view<Instance, Transparent>()) 
        renderMainInstance(reg, data.propShader, data, e_instance);

    // =====================
    // draw skybox
    // =====================
    // TODO

    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glDisable(GL_BLEND);
    // glCullFace(GL_BACK);
    // glEnable(GL_DEPTH_TEST);
    // glDepthMask(GL_FALSE);
    // glDepthFunc(GL_LEQUAL);
    // glDisable(GL_CULL_FACE);
    // glDisable(GL_POLYGON_OFFSET_FILL);

    // glBindFramebuffer(GL_FRAMEBUFFER, data.mainFBO.id);

    // glUseProgram(data.skyboxShader.id);
    // glUniformMatrix4fv(ogl::getUniform(data.skyboxShader, "u_viewMat"), 1, false, glm::value_ptr(viewMat));
    // glUniformMatrix4fv(ogl::getUniform(data.skyboxShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));
    // for(ecs::entity e : reg.view<Skybox>()) {
    //     drawSkybox(reg, data.skyboxShader, data, e);
    // }

    // ===================
    // OIT COMPOSITE PASS 
    // ===================

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, data.mainFBO.id);
    glUseProgram(data.oitCompositeShader.id);
    glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, data.oitAccumTexture.id);
    glActiveTexture(GL_TEXTURE0 + 1); glBindTexture(GL_TEXTURE_2D, data.oitRevealageTexture.id);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ======================================
    // HDR IMAGE / OTHER POSTPROCESSING PASS 
    // ======================================

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(data.screenShader.id);
    glActiveTexture(GL_TEXTURE0 + 0); glBindTexture(GL_TEXTURE_2D, data.mainFBOColor.id);

    // draw a quad (hard-coded in VSh)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
