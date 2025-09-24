#include "engine/renderer/engine_renderer/engineRenderer.hpp"

namespace engine::detail
{
    static void resizeAttachment(ogl::Framebuffer &fbo, ogl::Texture &texture, glm::uvec2 size, GLenum attachment = GL_COLOR_ATTACHMENT0, GLenum format = GL_RGBA32F)
    {
        if(texture.id != 0)
        {
            glDeleteTextures(1, &texture.id);
            texture.id = 0;
        }
        glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);

        if(texture.numSamples == 1)
            glTextureStorage2D(texture.id, 1, GL_RGBA32F, size.x, size.y);
        else
            glTextureStorage2DMultisample(texture.id, texture.numSamples, GL_RGBA32F, size.x, size.y, true);

        ENGINE_ASSERT_MSG(fbo.id != 0, "invalid fbo");
        glNamedFramebufferTexture(fbo.id, attachment, texture.id, 0);
        ENGINE_ASSERT_MSG(ogl::isComplete(fbo), "");
    }
    static void resizeAttachment(ogl::Framebuffer &fbo, ogl::Renderbuffer &rbo, glm::uvec2 size, GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT, GLenum format = GL_DEPTH24_STENCIL8)
    {
        if(rbo.id != 0)
        {
            glDeleteRenderbuffers(1, &rbo.id);
            rbo.id = 0;
        }
        glCreateRenderbuffers(1, &rbo.id);

        if(rbo.numSamples == 1)
            glNamedRenderbufferStorage(rbo.id, GL_DEPTH24_STENCIL8, size.x, size.y);
        else
            glNamedRenderbufferStorageMultisample(rbo.id, rbo.numSamples, GL_DEPTH24_STENCIL8, size.x, size.y);

        ENGINE_ASSERT_MSG(fbo.id != 0, "invalid fbo");
        glNamedFramebufferRenderbuffer(fbo.id, attachment, GL_RENDERBUFFER, rbo.id);
        ENGINE_ASSERT_MSG(ogl::isComplete(fbo), "");
    }
} // namespace engine::detail


void engine::EngineRenderer::draw(ecs::registry &reg)
{
    ENGINE_ASSERT_MSG(reg.view<detail::RendererData>().size() == 1, "forgot to call engine::setup()?");
    detail::RendererData &data = reg.get<detail::RendererData>(reg.view<detail::RendererData>().at(0));
    auto &camera = reg.get<engine::Camera>(m_context.e_camera);

    if(data.prevCamSize != camera.size) { // resize or initialize buffers / textures
        detail::resizeAttachment(data.oitFBO, data.oitAccumTexture, camera.size);
        detail::resizeAttachment(data.oitFBO, data.oitRevelageTexture, camera.size, GL_COLOR_ATTACHMENT1, GL_R8);

        detail::resizeAttachment(data.mainFBO, data.mainFBOColor, camera.size);
        detail::resizeAttachment(data.mainFBO, data.mainFBORBO, camera.size);
    }

    renderMain(reg, data);

    data.prevCamSize = camera.size;
}
