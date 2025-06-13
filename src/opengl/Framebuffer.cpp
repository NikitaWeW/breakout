#include "Framebuffer.hpp"

opengl::Framebuffer::Framebuffer(unsigned)
{
    glGenFramebuffers(1, &m_renderID);
}
opengl::Framebuffer::~Framebuffer()
{
    if(canDeallocate()) {
        glDeleteFramebuffers(1, &m_renderID);
    }
}

void opengl::Framebuffer::bind(unsigned slot) const noexcept
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_renderID);
}
bool opengl::Framebuffer::isComplete()
{
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void opengl::Framebuffer::attach(Texture const &texture, GLenum attachment)
{
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.getRenderID(), 0);
}
void opengl::Framebuffer::attach(TextureMS const &texture, GLenum attachment)
{
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, texture.getRenderID(), 0);
}
void opengl::Framebuffer::attach(Renderbuffer const &renderbuffer, GLenum attachment)
{
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.getRenderID());
}
void opengl::Framebuffer::attach(RenderbufferMS const &renderbuffer, GLenum attachment)
{
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.getRenderID());
}

opengl::Renderbuffer::Renderbuffer(unsigned)
{
    glGenRenderbuffers(1, &m_renderID);
}
opengl::Renderbuffer::~Renderbuffer()
{
    if(canDeallocate()) {
        glDeleteRenderbuffers(1, &m_renderID);
    }
}

void opengl::Renderbuffer::bind(unsigned slot) const noexcept { glBindRenderbuffer(GL_RENDERBUFFER, m_renderID); }

opengl::RenderbufferMS::RenderbufferMS(unsigned)
{
    glGenRenderbuffers(1, &m_renderID);
}
opengl::RenderbufferMS::~RenderbufferMS()
{
    if(canDeallocate()) {
        glDeleteRenderbuffers(1, &m_renderID);
    }
}

void opengl::RenderbufferMS::bind(unsigned slot) const noexcept { glBindRenderbuffer(GL_RENDERBUFFER, m_renderID); }
