#include <cassert>
#include "VertexBuffer.hpp"

opengl::VertexBuffer::VertexBuffer(size_t size, GLenum usage)
{
    glGenBuffers(1, &m_renderID);
    bind();
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
}

opengl::VertexBuffer::VertexBuffer(size_t size, void *data, GLenum usage)
{
    glGenBuffers(1, &m_renderID);
    bind();
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

opengl::VertexBuffer::~VertexBuffer()
{
    if(canDeallocate()) 
        glDeleteBuffers(1, &m_renderID);
}

void opengl::VertexBuffer::bind(unsigned) const noexcept { glBindBuffer(GL_ARRAY_BUFFER, m_renderID); }

size_t opengl::getSizeOfGLType(GLenum type)
{
    switch (type) {
        case GL_BYTE:            return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:   return sizeof(GLubyte);
        case GL_SHORT:           return sizeof(GLshort);
        case GL_UNSIGNED_SHORT:  return sizeof(GLushort);
        case GL_INT:             return sizeof(GLint);
        case GL_UNSIGNED_INT:    return sizeof(GLuint);
        case GL_FLOAT:           return sizeof(GLfloat);
        case GL_DOUBLE:          return sizeof(GLdouble);
        default: 
            assert(false && "type not supported");
            return 0;
    }
}

void opengl::VertexArray::addBuffer(VertexBuffer const &buffer, InterleavedVertexBufferLayout const &layout)
{
    bind();
    buffer.bind();
    unsigned offset = 0;
    for(InterleavedVertexBufferLayout::Element const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, layout.getStride(), reinterpret_cast<void const *>(offset));
        glEnableVertexAttribArray(m_vertexAttribIndex);
        offset += element.count * getSizeOfGLType(element.type);
        ++m_vertexAttribIndex;
    }
}
void opengl::VertexArray::addBuffer(VertexBuffer const &buffer, VertexBufferLayout const &layout)
{
    bind(); buffer.bind();
    for(VertexBufferLayout::Element const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, element.count * getSizeOfGLType(element.type), reinterpret_cast<void const *>(element.offset));
        glEnableVertexAttribArray(m_vertexAttribIndex);
        ++m_vertexAttribIndex;
    }
}
void opengl::VertexArray::addBuffer(VertexBuffer const &buffer, InstancingVertexBufferLayout const &layout)
{
    bind(); buffer.bind();
    unsigned offset = 0;
    for(auto const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, layout.getStride(), reinterpret_cast<void const *>(offset));
        glVertexAttribDivisor(m_vertexAttribIndex, element.divisor);
        glEnableVertexAttribArray(m_vertexAttribIndex);
        offset += element.count * getSizeOfGLType(element.type);
        ++m_vertexAttribIndex;
    }
}

void opengl::VertexArray::bind(unsigned) const noexcept { glBindVertexArray(m_renderID); }

opengl::VertexArray::~VertexArray()
{
    if(canDeallocate()) {
        glDeleteVertexArrays(1, &m_renderID);
    }
}

void opengl::InterleavedVertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
    m_stride += element.count * getSizeOfGLType(element.type);
}
void opengl::VertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
}
void opengl::InstancingVertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
    m_stride += element.count * getSizeOfGLType(element.type);
}