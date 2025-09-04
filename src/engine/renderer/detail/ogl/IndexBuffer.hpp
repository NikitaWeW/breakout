#pragma once
#include "Object.hpp"
#include <cstddef>
#include "glad/gl.h"

namespace ogl
{
    class IndexBuffer : public Object 
    {
    public:
        IndexBuffer() = default;
        IndexBuffer(size_t size, void const *data = nullptr, GLenum usage = GL_DYNAMIC_DRAW) noexcept;
        ~IndexBuffer();
    }; 
} // namespace ogl
