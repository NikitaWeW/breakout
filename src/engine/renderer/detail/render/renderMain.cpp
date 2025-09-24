#include "render.hpp"

void engine::renderer::detail::renderMain(ecs::registry &reg, detail::RendererData &data, renderer::RendererContext &context)
{
    auto &camera = reg.get<engine::Camera>(context.e_camera);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, camera.size.x, camera.size.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(data.plainColorShader.id);
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    for(ecs::entity e_model : reg.view<engine::renderer::Draw>())
    {
        ecs::entity e_mesh = reg.get<engine::renderer::ProcessedModel>(reg.get<engine::renderer::Draw>(e_model).model).data;
        detail::Mesh const &mesh = reg.get<detail::Mesh>(e_mesh);
        
        glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_model) ? reg.get<engine::ModelMatrix>(e_model).value : glm::mat4{1.0f};
        glm::mat4 normalMat = glm::transpose(glm::inverse(modelMat));
        
        glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
        glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_modelMat"), 1, false, glm::value_ptr(modelMat));
        // glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_boneMatrices"), 100, false, glm::value_ptr());
        glUniform1i(ogl::getUniform(data.plainColorShader, "u_animated"), mesh.animated);

        glBindVertexArray(mesh.vao.id);
        glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
    }
}
