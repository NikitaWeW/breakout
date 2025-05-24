#pragma once
#include "opengl/Object.hpp"
#include <cstddef>

namespace opengl
{
    class UniformBuffer : public Object 
    {
    public:
        UniformBuffer() = default;
        UniformBuffer(int) noexcept;
        ~UniformBuffer();

        void bind(unsigned slot = 0) const noexcept;
        void bindingPoint(unsigned index) const noexcept;
    };

    class SSBO : public Object
    {
    public:
        SSBO() = default;
        SSBO(int) noexcept;
        ~SSBO();

        void bind(unsigned slot = 0) const noexcept;
        void bindingPoint(unsigned index) const noexcept;
    };
} // namespace opengl
