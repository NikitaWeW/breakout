#pragma once
#include "Object.hpp"
#include <cstddef>
#include "glad/gl.h"

namespace opengl
{
    class IndexBuffer : public Object 
    {
    public:
        IndexBuffer() = default;
        IndexBuffer(size_t size, GLenum usage = GL_DYNAMIC_DRAW);
        IndexBuffer(size_t size, unsigned const *data, GLenum usage = GL_DYNAMIC_DRAW);
        ~IndexBuffer();
        void bind(unsigned slot = 0) const;
    }; 
} // namespace opengl
