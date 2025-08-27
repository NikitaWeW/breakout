#pragma once

#include "Object.hpp"
#include "glad/gl.h"
#include "Texture.hpp"

namespace ogl
{
    /**
     * @brief RAII wrapper for an OpenGL renderbuffer object.
     *
     * Manages the lifetime of an OpenGL renderbuffer handle,
     * automatically creating and deleting the underlying resource.
     * Inherits reference counting and deallocation logic from Object.
     */
    class Renderbuffer : public Object {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL renderbuffer is generated.
         */
        Renderbuffer() = default;

        /**
         * @brief Constructs and generates an OpenGL renderbuffer.
         *
         * The dummy unsigned parameter exists only to differentiate
         * this constructor overload; it does not affect the creation logic.
         */
        explicit Renderbuffer(unsigned);

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL renderbuffer if one was generated
         * and if no more references remain (canDeallocate()).
         */
        ~Renderbuffer();

        /**
         * @brief Binds this renderbuffer to the specified target.
         *
         * Calls glBindRenderbuffer(GL_RENDERBUFFER, m_renderID).
         *
         * @param slot Ignored for renderbuffers; present for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

    /**
     * @brief RAII wrapper for an OpenGL framebuffer object.
     *
     * Manages creation, binding, attachment, and deletion of
     * an OpenGL framebuffer. Inherits reference counting from Object.
     */
    class Framebuffer : public Object {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL framebuffer is generated.
         */
        Framebuffer() = default;

        /**
         * @brief Constructs and generates an OpenGL framebuffer.
         *
         * The dummy unsigned parameter exists only to differentiate
         * this constructor overload; it does not affect the creation logic.
         */
        explicit Framebuffer(unsigned);

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL framebuffer if one was generated
         * and if no more references remain (canDeallocate()).
         */
        ~Framebuffer();

        /**
         * @brief Binds this framebuffer to the specified target.
         *
         * Calls glBindFramebuffer(GL_FRAMEBUFFER, m_renderID).
         *
         * @param slot Ignored for framebuffers; present for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;

        /**
         * @brief Checks completeness of the currently bound framebuffer.
         *
         * Wraps glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE.
         *
         * @return true if the framebuffer is complete and ready for rendering.
         */
        bool isComplete();

        /**
         * @brief Attaches a 2D texture to this framebuffer.
         *
         * @param texture   The 2D texture to attach.
         * @param attachment The attachment point (default is GL_COLOR_ATTACHMENT0).
         */
        void attach(Texture const &texture, GLenum attachment = GL_COLOR_ATTACHMENT0);

        /**
         * @brief Attaches a multisample texture to this framebuffer.
         *
         * @param texture   The multisample texture to attach.
         * @param attachment The attachment point (default is GL_COLOR_ATTACHMENT0).
         */
        void attach(TextureMS const &texture, GLenum attachment = GL_COLOR_ATTACHMENT0);

        /**
         * @brief Attaches a cubemap face to this framebuffer.
         *
         * @param cubemap   The cubemap to attach.
         * @param attachment The attachment point (default is GL_COLOR_ATTACHMENT0).
         */
        void attach(Cubemap const &cubemap, GLenum attachment = GL_COLOR_ATTACHMENT0);

        /**
         * @brief Attaches a renderbuffer (e.g., depth/stencil) to this framebuffer.
         *
         * @param renderbuffer The renderbuffer to attach.
         * @param attachment   The attachment point
         *                     (default is GL_DEPTH_STENCIL_ATTACHMENT).
         */
        void attach(Renderbuffer const &renderbuffer, GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT);
    };
} // namespace ogl
