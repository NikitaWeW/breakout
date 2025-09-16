#include "render.hpp"

void engine::renderer::detail::renderMain(ecs::registry &reg, detail::RendererData &data, renderer::RendererContext &context)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(data.plainColorShader.id);

    auto &camera = reg.get<engine::Camera>(context.e_camera);

    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));
    
    for(ecs::entity e_processedModel : reg.view<engine::renderer::ProcessedModel>())
    {
        ecs::entity e_mesh = reg.get<engine::renderer::ProcessedModel>(e_processedModel).data;
        detail::Mesh const &mesh = reg.get<detail::Mesh>(e_mesh);
        
        glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_mesh) ? reg.get<engine::ModelMatrix>(e_mesh).value : glm::mat4{1.0f};
        glm::mat4 normalMat = glm::transpose(glm::inverse(modelMat));
        
        glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
        glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_modelMat"), 1, false, glm::value_ptr(modelMat));
        // glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_boneMatrices"), 100, false, glm::value_ptr());
        glUniform1i(ogl::getUniform(data.plainColorShader, "u_animated"), mesh.animated);

        glBindVertexArray(mesh.vao.id);
        glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
    }
}
