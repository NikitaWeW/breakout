#include "engine/Renderer/engineRenderer.hpp"
#include "../detail.hpp"

namespace ogl = engine::renderer::ogl;

void engine::EngineRenderer::draw(ecs::registry &reg)
{
    ENGINE_ASSERT_MSG(reg.view<renderer::RendererData>().size() == 1, "forgot to call engine::EngineRenderer::setup()?");
    renderer::RendererData &data = reg.get<renderer::RendererData>(reg.view<renderer::RendererData>().at(0));
    auto &camera = reg.get<engine::Camera>(data.context.e_camera);

    if(camera.size == glm::uvec2{0, 0})
        return;

    if(data.prevCamSize != camera.size) 
    { // resize or initialize buffers / textures
        ogl::attachment(data.mainFBO, data.mainFBOColor,        camera.size, GL_COLOR_ATTACHMENT0, GL_RGBA32F);
        ogl::attachment(data.oitFBO,  data.oitAccumTexture,     camera.size, GL_COLOR_ATTACHMENT1, GL_RGBA32F);
        ogl::attachment(data.oitFBO,  data.oitRevealageTexture, camera.size, GL_COLOR_ATTACHMENT2, GL_R16    );

        ogl::attachment(data.mainFBO, data.mainFBORBO, camera.size);
        glNamedFramebufferRenderbuffer(data.oitFBO.id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, data.mainFBORBO.id);
    }

    unsigned toDraw = glm::min<unsigned>(data.SM.lights.size() - data.SM.framesDrawn, glm::max<unsigned>(glm::ceil(data.SM.lights.size() / static_cast<float>(m_context.MAX_SHADOW_MAP_FRAMES)), 1));
    if(data.SM.refresh)
    {
        data.SM.framesDrawn = 0;
        toDraw = data.SM.lights.size();
        data.SM.refresh = false;
    }
    renderShadowMaps(reg, data, toDraw);
    renderMain(reg, data);

    data.prevCamSize = camera.size;
}
