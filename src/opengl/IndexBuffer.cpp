#include "IndexBuffer.hpp"

opengl::IndexBuffer::IndexBuffer(size_t size, GLenum usage) noexcept
{
    glGenBuffers(1, &m_renderID);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, usage);
}

opengl::IndexBuffer::IndexBuffer(size_t size, unsigned const *data, GLenum usage) noexcept
{
    glGenBuffers(1, &m_renderID);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
}

opengl::IndexBuffer::~IndexBuffer()
{
    if(canDeallocate())
        glDeleteBuffers(1, &m_renderID);
}

void opengl::IndexBuffer::bind(unsigned slot) const noexcept { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderID); }