#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"

void bindTexture(int location, ogl::Texture texture, ogl::Texture defaultTexture, unsigned &slot)
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
void bindTextures(ogl::Program const &program, engine::detail::MaterialTextures const &textures, ogl::Texture defaultTexture)
{
    unsigned slot = 0;
    bindTexture(ogl::getUniform(program, "u_material.albedo"), textures.albedo, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.metallic"), textures.metallic, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.roughness"), textures.roughness, defaultTexture, slot);
    bindTexture(ogl::getUniform(program, "u_material.normal"), textures.normal, defaultTexture, slot);
}

void engine::EngineRenderer::renderMain(ecs::registry &reg, detail::RendererData &data)
{
    auto &camera = reg.get<engine::Camera>(m_context.e_camera);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, camera.size.x, camera.size.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(data.plainColorShader.id);
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glCullFace(GL_BACK);
    
    for(ecs::entity e_instance : reg.view<engine::Instance>())
    {
        detail::Model const &model = reg.get<detail::Model>(reg.get<detail::ProcessedModel>(reg.get<engine::Instance>(e_instance).e_model).data);
        std::vector<glm::mat4> const *boneMatrices = reg.has<CurrentAnimation>(e_instance) ? &reg.get<CurrentAnimation>(e_instance).boneMatrices : &model.skeleton.tposeTransform;
        ENGINE_ASSERT(boneMatrices->size() == model.skeleton.boneMap.size());

        for(auto const &mesh : model.meshes)
        {
            glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_instance) ? reg.get<engine::ModelMatrix>(e_instance).value : glm::mat4{1.0f};
            glm::mat4 normalMat = glm::transpose(glm::inverse(modelMat));

            bindTextures(data.plainColorShader, mesh.textures, data.defaultTexture);
            
            glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
            glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
            if(model.animated)
                glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_boneMatrices"), boneMatrices->size(), false, glm::value_ptr(boneMatrices->front()));
            glUniform1i(ogl::getUniform(data.plainColorShader, "u_animated"), model.animated);
    
            glBindVertexArray(mesh.vao.id);
            glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
        }
    }
}
