#pragma once
#include "Object.hpp"
#include <cstddef>

namespace ogl
{
    /**
     * @brief RAII wrapper for an OpenGL uniform buffer object (UBO).
     *
     * Manages creation, binding, and deletion of a GL_UNIFORM_BUFFER.
     * Inherits reference counting and deallocation logic from Object.
     */
    class UniformBuffer : public Object 
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL uniform buffer is generated.
         */
        UniformBuffer() = default;

        /**
         * @brief Creates and generates an OpenGL uniform buffer.
         *
         * Generates a buffer object.
         */
        explicit UniformBuffer(int) noexcept;

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL uniform buffer if generated
         * and if canDeallocate() returns true.
         */
        ~UniformBuffer();

        /**
         * @brief Binds this buffer as the active uniform buffer.
         *
         * Calls glBindBuffer(GL_UNIFORM_BUFFER, m_renderID).
         *
         * @param slot Ignored for uniform buffers; present for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;

        /**
         * @brief Binds the uniform buffer to a binding point.
         *
         * Calls glBindBufferBase(GL_UNIFORM_BUFFER, index, m_renderID).
         *
         * @param index The binding point index.
         */
        void bindingPoint(unsigned index) const noexcept;
    };

    /**
     * @brief RAII wrapper for an OpenGL shader storage buffer object (SSBO).
     *
     * Manages creation, binding, and deletion of a GL_SHADER_STORAGE_BUFFER.
     * Inherits reference counting and deallocation logic from Object.
     */
    class SSBO : public Object
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL shader storage buffer is generated.
         */
        SSBO() = default;

        /**
         * @brief Creates and generates an OpenGL shader storage buffer.
         *
         * Generates a buffer object.
         */
        explicit SSBO(int) noexcept;

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL shader storage buffer if generated
         * and if canDeallocate() returns true.
         */
        ~SSBO();

        /**
         * @brief Binds this buffer as the active shader storage buffer.
         *
         * Calls glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_renderID).
         *
         * @param slot Ignored for SSBOs; present for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;

        /**
         * @brief Binds the shader storage buffer to a binding point.
         *
         * Calls glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_renderID).
         *
         * @param index The binding point index.
         */
        void bindingPoint(unsigned index) const noexcept;
    };
} // namespace opengl
