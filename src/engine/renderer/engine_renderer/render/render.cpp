
#include "engine/Header/Config.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "../detail.hpp"

void engine::EngineRendererImpl::draw()
{
    if(!mShaders.valid()) return;
    // ENGINE_ASSERT_MSG(reg.view<renderer::RendererData>().size() == 1, "forgot to call engine::EngineRenderer::setup()?");
    // renderer::RendererData &data = reg.get<renderer::RendererData>(reg.view<renderer::RendererData>().at(0));
    auto &camera = mReg->get<engine::Camera>(mConfig.e_camera);

    if(camera.size == glm::uvec2{0, 0})
        return;

    if(mPrevCamSize != camera.size) 
    { // resize or initialize buffers / textures
        ogl::attachment(mMainFbo, mMainFboColor,        camera.size, GL_COLOR_ATTACHMENT0, GL_RGBA32F);
        ogl::attachment(mOitFbo,  mOitAccumTexture,     camera.size, GL_COLOR_ATTACHMENT1, GL_RGBA32F);
        ogl::attachment(mOitFbo,  mOitRevealageTexture, camera.size, GL_COLOR_ATTACHMENT2, GL_R16    );

        ogl::attachment(mMainFbo, mMainFboRbo, camera.size);
        glNamedFramebufferRenderbuffer(mOitFbo.id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mMainFboRbo.id);
    }

    processData();

    auto const &atlas = mLightManager.getAtlas();
    unsigned toDraw = glm::min<unsigned>(atlas.viewports.size() - atlas.framesDrawn, glm::max<unsigned>(glm::ceil(atlas.viewports.size() / static_cast<float>(mConfig.MAX_SHADOW_MAP_FRAMES)), 1));

    if(atlas.refreshed) // Draw all
        toDraw = atlas.viewports.size();
    renderShadowMaps(toDraw);
    renderMain();

    mPrevCamSize = camera.size;
}
void engine::EngineRenderer::draw()
{
    unwrap().draw();
}