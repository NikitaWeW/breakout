#include "render.hpp"

namespace engine::renderer::detail
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

        ENGINE_ASSERT(fbo.id != 0, "invalid fbo");
        glNamedFramebufferTexture(fbo.id, attachment, texture.id, 0);
        // ENGINE_ASSERT(ogl::isComplete(fbo), "");
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

        ENGINE_ASSERT(fbo.id != 0, "invalid fbo");
        glNamedFramebufferRenderbuffer(fbo.id, attachment, GL_RENDERBUFFER, rbo.id);
        // ENGINE_ASSERT(ogl::isComplete(fbo), "");
    }
} // namespace engine::renderer::detail


void engine::renderer::detail::render(ecs::registry &reg)
{
    ENGINE_ASSERT(reg.view<detail::RendererData>().size() == 1 && reg.view<RendererContext>().size() == 1, "forgot to call engine::renderer::setup() / called more than once?");
    detail::RendererData &data = reg.get<detail::RendererData>(reg.view<detail::RendererData>().at(0));
    RendererContext &context = reg.get<RendererContext>(reg.view<RendererContext>().at(0));
    auto &camera = reg.get<engine::Camera>(context.e_camera);

    if(data.prevCamSize != camera.size) { // resize or initialize buffers / textures
        detail::resizeAttachment(data.oitFBO, data.oitAccumTexture, camera.size);
        detail::resizeAttachment(data.oitFBO, data.oitRevelageTexture, camera.size, GL_COLOR_ATTACHMENT1, GL_R8);

        detail::resizeAttachment(data.mainFBO, data.mainFBOColor, camera.size);
        detail::resizeAttachment(data.mainFBO, data.mainFBORBO, camera.size);
    }

    detail::renderMain(reg, data, context);

    data.prevCamSize = camera.size;
}
