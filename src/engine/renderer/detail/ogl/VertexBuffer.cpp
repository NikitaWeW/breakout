#include <cassert>
#include "VertexBuffer.hpp"

ogl::VertexBuffer::VertexBuffer(size_t size, void const *data, GLenum usage)
{
    glCreateBuffers(1, &m_renderID);
    glNamedBufferData(m_renderID, size, data, usage);
}
ogl::VertexBuffer::~VertexBuffer()
{
    if(canDeallocate()) 
        glDeleteBuffers(1, &m_renderID);
}

size_t ogl::getSizeOfGLType(GLenum type)
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

// TODO: switch this to ogl 4.5 dsa
void ogl::VertexArray::addBuffer(VertexBuffer const &buffer, InterleavedVertexBufferLayout const &layout)
{
    glBindVertexArray(m_renderID);
    glBindBuffer(GL_ARRAY_BUFFER, m_renderID);
    
    unsigned offset = 0;
    for(InterleavedVertexBufferLayout::Element const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, layout.getStride(), reinterpret_cast<void const *>(offset));
        glEnableVertexAttribArray(m_vertexAttribIndex);
        offset += element.count * getSizeOfGLType(element.type);
        ++m_vertexAttribIndex;
    }
}
void ogl::VertexArray::addBuffer(VertexBuffer const &buffer, VertexBufferLayout const &layout)
{
    glBindVertexArray(m_renderID);
    glBindBuffer(GL_ARRAY_BUFFER, m_renderID);
    
    for(VertexBufferLayout::Element const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, element.count * getSizeOfGLType(element.type), reinterpret_cast<void const *>(element.offset));
        glEnableVertexAttribArray(m_vertexAttribIndex);
        ++m_vertexAttribIndex;
    }
}
void ogl::VertexArray::addBuffer(VertexBuffer const &buffer, InterleavedInstancingVertexBufferLayout const &layout)
{
    glBindVertexArray(m_renderID);
    glBindBuffer(GL_ARRAY_BUFFER, m_renderID);
    
    unsigned offset = 0;
    for(auto const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, layout.getStride(), reinterpret_cast<void const *>(offset));
        glVertexAttribDivisor(m_vertexAttribIndex, element.divisor);
        glEnableVertexAttribArray(m_vertexAttribIndex);
        offset += element.count * getSizeOfGLType(element.type);
        ++m_vertexAttribIndex;
    }
}
void ogl::VertexArray::addBuffer(VertexBuffer const &buffer, InstancingVertexBufferLayout const &layout)
{
    glBindVertexArray(m_renderID);
    glBindBuffer(GL_ARRAY_BUFFER, m_renderID);
    
    for(InstancingVertexBufferLayout::Element const &element : layout.getElements()) {
        glVertexAttribPointer(m_vertexAttribIndex, element.count, element.type, false, element.count * getSizeOfGLType(element.type), reinterpret_cast<void const *>(element.offset));
        glEnableVertexAttribArray(m_vertexAttribIndex);
        glVertexAttribDivisor(m_vertexAttribIndex, element.divisor);
        ++m_vertexAttribIndex;
    }
}

ogl::VertexArray::~VertexArray()
{
    if(canDeallocate()) {
        glDeleteVertexArrays(1, &m_renderID);
    }
}

ogl::InterleavedVertexBufferLayout::InterleavedVertexBufferLayout(std::initializer_list<Element> const &elements) : InterleavedVertexBufferLayout(std::vector(elements)) {}
ogl::VertexBufferLayout::VertexBufferLayout(std::initializer_list<Element> const &elements) : VertexBufferLayout(std::vector(elements)) {}
ogl::InterleavedInstancingVertexBufferLayout::InterleavedInstancingVertexBufferLayout(std::initializer_list<Element> const &elements) : InterleavedInstancingVertexBufferLayout(std::vector(elements)) {}
ogl::InstancingVertexBufferLayout::InstancingVertexBufferLayout(std::initializer_list<Element> const &elements) : InstancingVertexBufferLayout(std::vector(elements)) {}

ogl::InterleavedVertexBufferLayout::InterleavedVertexBufferLayout(std::vector<Element> const &elements)
{
    for(Element const &element : elements) {
        push(element);
    }
}
ogl::VertexBufferLayout::VertexBufferLayout(std::vector<Element> const &elements)
{
    for(Element const &element : elements) {
        push(element);
    }
}
ogl::InterleavedInstancingVertexBufferLayout::InterleavedInstancingVertexBufferLayout(std::vector<Element> const &elements)
{
    for(Element const &element : elements) {
        push(element);
    }
}
ogl::InstancingVertexBufferLayout::InstancingVertexBufferLayout(std::vector<Element> const &elements)
{
    for(Element const &element : elements) {
        push(element);
    }
}

void ogl::InterleavedVertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
    m_stride += element.count * getSizeOfGLType(element.type);
}
void ogl::VertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
}
void ogl::InterleavedInstancingVertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
    m_stride += element.count * getSizeOfGLType(element.type);
}
void ogl::InstancingVertexBufferLayout::push(Element const &element)
{
    m_elements.push_back(element);
}
