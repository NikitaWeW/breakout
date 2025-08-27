#pragma once

#include "Object.hpp"
#include <cstddef>
#include "glad/gl.h"

namespace ogl
{
    /**
     * @brief RAII wrapper for an OpenGL index buffer (element array buffer).
     *
     * Manages creation, binding, and deletion of an OpenGL element
     * array buffer (GL_ELEMENT_ARRAY_BUFFER). Inherits reference counting
     * and deallocation logic from Object.
     */
    class IndexBuffer : public Object 
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL buffer is generated.
         */
        IndexBuffer() = default;

        /**
         * @brief Constructs and allocates an OpenGL index buffer.
         *
         * Generates a buffer of the given size without initializing its contents.
         *
         * @param size  The size in bytes to allocate for the index buffer.
         * @param usage The expected usage pattern of the data store
         *              (e.g., GL_STATIC_DRAW, GL_DYNAMIC_DRAW).
         */
        IndexBuffer(size_t size, GLenum usage = GL_DYNAMIC_DRAW) noexcept;

        /**
         * @brief Constructs and initializes an OpenGL index buffer.
         *
         * Generates a buffer of the given size and uploads the provided data.
         *
         * @param size  The size in bytes to allocate for the index buffer.
         * @param data  Pointer to the initial index data to upload.
         * @param usage The expected usage pattern of the data store
         *              (e.g., GL_STATIC_DRAW, GL_DYNAMIC_DRAW).
         */
        IndexBuffer(size_t size, void const *data, GLenum usage = GL_DYNAMIC_DRAW) noexcept;

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL index buffer if one was generated
         * and if no more references remain (canDeallocate()).
         */
        ~IndexBuffer();

        /**
         * @brief Binds this buffer as the element array buffer.
         *
         * Calls glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderID).
         *
         * @param slot Ignored for index buffers; present for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;
    }; 
} // namespace ogl
