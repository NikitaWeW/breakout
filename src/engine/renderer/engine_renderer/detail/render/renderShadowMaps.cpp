#include "../detail.hpp"
#include "../../engineRenderer.hpp"

namespace ogl = engine::renderer::ogl;
using namespace engine;

void EngineRenderer::renderShadowMaps(ecs::registry &reg, renderer::RendererData &data)
{
    glBindFramebuffer(GL_FRAMEBUFFER, data.SMAtlas.fbo.id);
    glViewport(0, 0, data.SMAtlas.size.x, data.SMAtlas.size.y);

    glUseProgram(data.shaders.depthMapShader.id);
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, data.mainFBO.id);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUniform1i(ogl::getUniform(data.shaders.depthMapShader, "u_transparent"), false);
    for(ecs::entity e_instance : reg.view<Instance>(ecs::exclude_t<Transparent>{}))
    {
    }
}