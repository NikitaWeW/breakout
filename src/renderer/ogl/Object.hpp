#pragma once

#include "Resource.hpp"

namespace ogl
{
    /**
     * @brief Abstract base class representing an OpenGL object resource.
     *
     * Inherits reference counting and deallocation logic from Resource.
     * Manages an OpenGL render ID and requires derived classes to implement
     * their specific bind operation.
     */
    class Object : public Resource
    {
    protected:
        /**
         * @brief OpenGL render identifier.
         *
         * Holds the GLuint handle assigned by OpenGL for this object
         * (e.g., buffer, texture, vertex array). A zero value indicates
         * no valid OpenGL resource is bound.
         */
        unsigned m_renderID = 0;
    public:
        /**
         * @brief Determines if the object can be safely deallocated.
         *
         * Combines the base class's reference and ownership checks
         * with ensuring that the OpenGL render ID is valid (non-zero).
         *
         * @return true if no more references exist and m_renderID is non-zero.
         */
        inline bool canDeallocate() const
        {
            return Resource::canDeallocate() && m_renderID;
        }

        /**
         * @brief Retrieves the OpenGL render ID.
         *
         * @return The current render ID (zero if uninitialized).
         */
        inline unsigned getRenderID() const noexcept
        {
            return m_renderID;
        }

        /**
         * @brief Retrieves a modifiable reference to the render ID.
         *
         * Allows clients to assign or reset the OpenGL handle.
         *
         * @return Reference to m_renderID.
         */
        inline unsigned &getRenderID() noexcept
        {
            return m_renderID;
        }

        /**
         * @brief Binds the OpenGL object to the specified slot.
         *
         * Must be implemented by derived classes to perform the correct
         * OpenGL bind call (e.g., glBindBuffer, glBindTexture).
         *
         * @param slot The binding slot or unit (default is 0).
         */
        virtual void bind(unsigned slot = 0) const noexcept = 0;
    };

} // namespace ogl
