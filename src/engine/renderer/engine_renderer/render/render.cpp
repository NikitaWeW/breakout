#include "engine/Renderer/EngineRenderer.hpp"
#include "../detail.hpp"

namespace ogl = engine::ogl;

void engine::EngineRendererImpl::draw()
{
    if(!shaders.valid()) return;
    // ENGINE_ASSERT_MSG(reg.view<renderer::RendererData>().size() == 1, "forgot to call engine::EngineRenderer::setup()?");
    // renderer::RendererData &data = reg.get<renderer::RendererData>(reg.view<renderer::RendererData>().at(0));
    auto &camera = reg->get<engine::Camera>(config.e_camera);

    if(camera.size == glm::uvec2{0, 0})
        return;

    if(prevCamSize != camera.size) 
    { // resize or initialize buffers / textures
        ogl::attachment(mainFBO, mainFBOColor,        camera.size, GL_COLOR_ATTACHMENT0, GL_RGBA32F);
        ogl::attachment(oitFBO,  oitAccumTexture,     camera.size, GL_COLOR_ATTACHMENT1, GL_RGBA32F);
        ogl::attachment(oitFBO,  oitRevealageTexture, camera.size, GL_COLOR_ATTACHMENT2, GL_R16    );

        ogl::attachment(mainFBO, mainFBORBO, camera.size);
        glNamedFramebufferRenderbuffer(oitFBO.id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mainFBORBO.id);
    }

    processData();

    auto const &atlas = mLightManager.getAtlas();
    unsigned toDraw = glm::min<unsigned>(atlas.viewports.size() - atlas.framesDrawn, glm::max<unsigned>(glm::ceil(atlas.viewports.size() / static_cast<float>(config.MAX_SHADOW_MAP_FRAMES)), 1));

    if(atlas.refreshed) // Draw all
        toDraw = atlas.viewports.size();
    renderShadowMaps(toDraw);
    renderMain();

    prevCamSize = camera.size;
}
void engine::EngineRenderer::draw(Registry &reg)
{
    // unwrap().reg = &reg;
    unwrap().draw();
}