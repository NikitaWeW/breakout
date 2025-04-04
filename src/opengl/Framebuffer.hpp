#pragma once
#include "Object.hpp"
#include "glad/gl.h"
#include "opengl/Texture.hpp"

namespace opengl
{
    class Renderbuffer : public Object {
    public:
        Renderbuffer() = default;
        Renderbuffer(unsigned width, unsigned height, GLenum format);
        ~Renderbuffer();
    
        void bind(unsigned slot = 0) const noexcept;
    };
    class RenderbufferMS : public Object {
    public:
        RenderbufferMS() = default;
        RenderbufferMS(unsigned width, unsigned height, unsigned samples, GLenum format);
        ~RenderbufferMS();
    
        void bind(unsigned slot = 0) const noexcept;
    };

    class Framebuffer : public Object {
    protected:
        mutable unsigned m_renderID = 0;
    public:
        Framebuffer() = default;
        ~Framebuffer();
        void bind(unsigned slot = 0) const noexcept;
        bool isComplete();
        void attach(Texture const &texture, GLenum attachment = GL_COLOR_ATTACHMENT0);
        void attach(TextureMS const &texture, GLenum attachment = GL_COLOR_ATTACHMENT0);
        void attach(Renderbuffer const &renderbuffer, GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT);
        void attach(RenderbufferMS const &renderbuffer, GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT);
    };
} // namespace opengl
