#include "../detail.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace engine;

void EngineRendererImpl::drawSM(size_t first, size_t count)
{
    glViewportArrayv(0, count, reinterpret_cast<float const *>(&SM.viewports.at(first)));
    
    glEnable(GL_SCISSOR_TEST);
    for(size_t i = first; i < first + count; ++i)
    {
        auto viewport = SM.viewports[i];
        glScissor(viewport.pos.x, viewport.pos.y, viewport.size.x, viewport.size.y);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);

    // TODO: transparent object shadows
    for(ecs::entity e_instance : reg->view<Instance>(ecs::exclude_t</**Transparent, **/NoShadow>{}))
    {
        auto const &instance = reg->get<engine::Instance>(e_instance);
        ENGINE_ASSERT_MSG(reg->has<renderer::ProcessedTag>(instance.e_model), "Forgot to call engine::EngineRenderer::processData()?");
        renderer::Model const &model = reg->get<renderer::Model>(instance.e_model);
        bool animated = model.animated && reg->has<CurrentAnimation>(e_instance);

        for(auto const &mesh : model.meshes)
        {
            glm::mat4 modelMat = reg->has<engine::Transform>(e_instance) ? reg->get<engine::Transform>(e_instance).getMat() : glm::mat4{1.0f};

            glUniformMatrix4fv(ogl::getUniform(shaders.depthMapShader, "u_modelMat"),  1, false, glm::value_ptr(modelMat));
            if(animated)
            {
                auto const &boneMatrices = reg->get<CurrentAnimation>(e_instance).boneMatrices;
                ENGINE_ASSERT(boneMatrices.size() == model.skeleton.boneMap.size());
                glUniformMatrix4fv(ogl::getUniform(shaders.depthMapShader, "u_boneMatrices"), boneMatrices.size(), false, glm::value_ptr(boneMatrices.front())); // TODO: switch to ssbo or ubo.
            }
            glUniform1i(ogl::getUniform(shaders.depthMapShader, "u_animated"), animated);

            glUniform1ui(ogl::getUniform(shaders.depthMapShader, "u_first"), first);

            glBindVertexArray(mesh.vao.id);
            glDrawElementsInstanced(mesh.mode, mesh.count, GL_UNSIGNED_INT, nullptr, count);
        }
    }
}
static unsigned getMaxViewports()
{
    int res;
    glGetIntegerv(GL_MAX_VIEWPORTS, &res);
    return static_cast<unsigned>(res);
}

void EngineRendererImpl::renderShadowMaps(unsigned toDraw)
{
    // RENDERER_TRACE("Drawing {} / {} shadow maps", toDraw, SM.lights.size());
    
    glBindFramebuffer(GL_FRAMEBUFFER, SM.atlas.fbo.id);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glDisable(GL_SCISSOR_TEST);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SM.lightsSSBO.id);
    glUseProgram(shaders.depthMapShader.id);

    // Annoyingly this approach is limited by GL_MAX_VIEWPORTS, so the rendering is done in batches.
    unsigned maxDraw = getMaxViewports();

    for(unsigned batch = 0; batch <= toDraw / maxDraw; ++batch)
    {
        unsigned thisDraw = glm::min(toDraw - maxDraw * batch, maxDraw);
        drawSM(reg, data, SM.framesDrawn, thisDraw);
        SM.framesDrawn += thisDraw;
    }

    if(SM.framesDrawn >= SM.lights.size())
        SM.framesDrawn = 0;
}