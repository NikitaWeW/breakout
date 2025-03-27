#include "Framebuffer.hpp"

opengl::Framebuffer::~Framebuffer()
{
    if(canDeallocate()) {
        glDeleteFramebuffers(1, &m_renderID);
    }
}

void opengl::Framebuffer::bind(unsigned slot) const noexcept
{
    if(m_renderID == 0) {
        glGenFramebuffers(1, &m_renderID);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_renderID);
}

bool opengl::Framebuffer::isComplete()
{
    bind();
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void opengl::Framebuffer::attach(Texture const &texture, GLenum attachment)
{
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.getRenderID(), 0);
}

void opengl::Framebuffer::attach(Renderbuffer const &renderbuffer, GLenum attachment)
{
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.getRenderID());
}

opengl::Renderbuffer::Renderbuffer(unsigned width, unsigned height, GLenum format)
{
    glGenRenderbuffers(1, &m_renderID);
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
}

opengl::Renderbuffer::~Renderbuffer()
{
    if(canDeallocate()) {
        glDeleteRenderbuffers(1, &m_renderID);
    }
}

void opengl::Renderbuffer::bind(unsigned slot) const noexcept { glBindRenderbuffer(GL_RENDERBUFFER, m_renderID); }
