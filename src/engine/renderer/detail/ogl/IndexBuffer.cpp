#include "IndexBuffer.hpp"

ogl::IndexBuffer::IndexBuffer(size_t size, void const *data, GLenum usage) noexcept
{
    glCreateBuffers(1, &m_renderID);
    glNamedBufferData(m_renderID, size, data, usage);
}

ogl::IndexBuffer::~IndexBuffer()
{
    if(canDeallocate())
        glDeleteBuffers(1, &m_renderID);
}
