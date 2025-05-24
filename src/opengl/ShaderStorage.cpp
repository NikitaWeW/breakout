#include "ShaderStorage.hpp"
#include "glad/gl.h"

opengl::UniformBuffer::UniformBuffer(int) noexcept
{
    glGenBuffers(1, &m_renderID);
}

opengl::UniformBuffer::~UniformBuffer()
{
    if(canDeallocate()) {
        glDeleteBuffers(1, &m_renderID);
    }
}

void opengl::UniformBuffer::bind(unsigned slot) const noexcept { glBindBuffer(GL_UNIFORM_BUFFER, m_renderID); }
void opengl::UniformBuffer::bindingPoint(unsigned index) const noexcept { glBindBufferBase(GL_UNIFORM_BUFFER, index, m_renderID); }

opengl::SSBO::SSBO(int) noexcept
{
    glCreateBuffers(1, &m_renderID);
}

opengl::SSBO::~SSBO()
{
    if(canDeallocate()) {
        glDeleteBuffers(1, &m_renderID);
    }
}

void opengl::SSBO::bind(unsigned slot) const noexcept { glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderID); }
void opengl::SSBO::bindingPoint(unsigned index) const noexcept { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_renderID); }
