#pragma once
#include "opengl/Object.hpp"
#include <cstddef>

namespace opengl
{
    class UniformBuffer : public Object 
    {
        UniformBuffer() = default;
        UniformBuffer(size_t size) noexcept;
        ~UniformBuffer();

        void bind(unsigned slot = 0) const noexcept;
        void bindingPoint(unsigned index) const noexcept;
    };

    class SSBO : public Object
    {
        SSBO() = default;
        SSBO(size_t size) noexcept;
        ~SSBO();

        void bind(unsigned slot = 0) const noexcept;
        void bindingPoint(unsigned index) const noexcept;
    };
} // namespace opengl
