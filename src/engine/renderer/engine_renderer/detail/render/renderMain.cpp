#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"

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
    glCullFace(GL_BACK);
    
    for(ecs::entity e_draw : reg.view<engine::Draw>())
    {
        detail::Model const &model = reg.get<detail::Model>(reg.get<detail::ProcessedModel>(reg.get<engine::Draw>(e_draw).model).data);
        for(auto const &mesh : model.meshes)
        {
            glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_draw) ? reg.get<engine::ModelMatrix>(e_draw).value : glm::mat4{1.0f};
            glm::mat4 normalMat = glm::transpose(glm::inverse(modelMat));
            
            glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_normalMat"), 1, false, glm::value_ptr(normalMat));
            glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
            if(model.animated)
                glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_boneMatrices"), model.skeleton.tposeTransform.size(), false, glm::value_ptr(model.skeleton.tposeTransform.at(0)));
            glUniform1i(ogl::getUniform(data.plainColorShader, "u_animated"), model.animated);
    
            glBindVertexArray(mesh.vao.id);
            glDrawElements(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr);
        }
    }
}
