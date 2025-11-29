#include "../detail.hpp"
#include "../../engineRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace ogl = engine::renderer::ogl;
using namespace engine;

void EngineRenderer::renderShadowMaps(ecs::registry &reg, renderer::RendererData &data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, data.SMAtlas.fbo.id);
    glViewport(0, 0, data.SMAtlas.size.x, data.SMAtlas.size.y);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glViewport(0, 0, data.SMAtlas.size.x, data.SMAtlas.size.y);
    glDisable(GL_SCISSOR_TEST);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, data.drawLightsSSBO.id);
    glUseProgram(data.shaders.depthMapShader.id);
    glUniform2uiv(ogl::getUniform(data.shaders.depthMapShader, "u_atlasSize"), 1, glm::value_ptr(data.SMAtlas.size));
    
    // TODO: transparent object shadows
    for(ecs::entity e_instance : reg.view<Instance>(ecs::exclude_t</**Transparent, **/NoShadow>{}))
    {
        auto const &instance = reg.get<engine::Instance>(e_instance);
        ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedModel>(instance.e_model), "Forgot to call engine::EngineRenderer::processData()?");
        renderer::Model const &model = reg.get<renderer::Model>(instance.e_model);
        bool animated = model.animated && reg.has<CurrentAnimation>(e_instance);

        for(auto const &mesh : model.meshes)
        {
            glm::mat4 modelMat = reg.has<engine::ModelMatrix>(e_instance) ? reg.get<engine::ModelMatrix>(e_instance) : glm::mat4{1.0f};

            glUniformMatrix4fv(ogl::getUniform(data.shaders.depthMapShader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
            if(animated)
            {
                auto const &boneMatrices = reg.get<CurrentAnimation>(e_instance).boneMatrices;
                ENGINE_ASSERT(boneMatrices.size() == model.skeleton.boneMap.size());
                glUniformMatrix4fv(ogl::getUniform(data.shaders.depthMapShader, "u_boneMatrices"), boneMatrices.size(), false, glm::value_ptr(boneMatrices.front())); // TODO: switch to ssbo or ubo.
            }
            glUniform1i(ogl::getUniform(data.shaders.depthMapShader, "u_animated"), animated);

            glBindVertexArray(mesh.vao.id);
            glDrawElementsInstanced(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr, data.drawLights.size());
        }
    }
}