#pragma once
#include "Object.hpp"
#include <cstddef>

namespace ogl
{
    class UniformBuffer : public Object 
    {
    public:
        UniformBuffer() = default;
        explicit UniformBuffer(int) noexcept;
        ~UniformBuffer();

        void bindingPoint(unsigned index) const noexcept;
    };

    class SSBO : public Object
    {
    public:
        SSBO() = default;
        explicit SSBO(int) noexcept;
        ~SSBO();

        void bindingPoint(unsigned index) const noexcept;
    };
} // namespace opengl
