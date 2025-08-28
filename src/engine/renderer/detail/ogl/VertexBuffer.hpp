#pragma once

#include "Object.hpp"
#include "glad/gl.h"
#include <cstddef>
#include <vector>

namespace ogl
{
    /**
     * @brief Returns the size in bytes of a given OpenGL data type.
     *
     * Useful when calculating buffer strides, offsets, and element sizes.
     *
     * @param type The GLenum representing the OpenGL type (e.g., GL_UNSIGNED_INT).
     * @return The size in bytes of the specified GL type.
     */
    size_t getSizeOfGLType(GLenum type);

    /**
     * @brief RAII wrapper for an OpenGL vertex buffer object (VBO).
     *
     * Manages creation, data allocation, binding, and deletion of a
     * GL_ARRAY_BUFFER. Inherits reference counting and deallocation logic
     * from Object.
     */
    class VertexBuffer : public Object 
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL buffer is generated.
         */
        VertexBuffer() = default;

        /**
         * @brief Creates an empty vertex buffer of a given size.
         *
         * Generates a buffer and allocates GPU storage without initializing data.
         *
         * @param size  Size in bytes to allocate for the buffer.
         * @param usage Expected data usage pattern (e.g., GL_STATIC_DRAW).
         */
        explicit VertexBuffer(size_t size, GLenum usage = GL_DYNAMIC_DRAW);

        /**
         * @brief Creates and initializes a vertex buffer with data.
         *
         * Generates a buffer, allocates storage, and uploads the provided data.
         *
         * @param size  Size in bytes of the data to upload.
         * @param data  Pointer to the source data.
         * @param usage Expected data usage pattern (e.g., GL_STATIC_DRAW).
         */
        explicit VertexBuffer(size_t size, void const *data, GLenum usage = GL_DYNAMIC_DRAW);

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL vertex buffer if generated
         * and if canDeallocate() returns true.
         */
        ~VertexBuffer();

        /**
         * @brief Binds this buffer as the current GL_ARRAY_BUFFER.
         *
         * Calls glBindBuffer(GL_ARRAY_BUFFER, m_renderID).
         *
         * @param slot Ignored for vertex buffers; included for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

    /**
     * @brief Layout description for interleaved vertex attributes.
     *
     * Defines a sequence of attribute elements (count + type) and
     * automatically computes the stride for interleaving.
     */
    class InterleavedVertexBufferLayout 
    {
    public:
        /**
         * @brief Single element description in an interleaved layout.
         */
        struct Element {
            unsigned count;  ///< Number of components (e.g., 3 for vec3).
            unsigned type;   ///< OpenGL type (e.g., GL_FLOAT).
        };

    private:
        std::vector<Element> m_elements; ///< Sequence of interleaved elements.
        unsigned m_stride = 0;           ///< Total stride in bytes.

    public:
        /**
         * @brief Default constructor.
         */
        InterleavedVertexBufferLayout() = default;

        /**
         * @brief Constructs layout from an initializer list of elements.
         *
         * @param elements List of Element descriptors.
         */
        InterleavedVertexBufferLayout(std::initializer_list<Element> const &elements);

        /**
         * @brief Constructs layout from a vector of elements.
         *
         * @param elements Vector of Element descriptors.
         */
        InterleavedVertexBufferLayout(std::vector<Element> const &elements);

        /**
         * @brief Destructor.
         */
        ~InterleavedVertexBufferLayout() = default;

        /**
         * @brief Appends a new element to the layout and updates stride.
         *
         * @param element The Element descriptor to add.
         */
        void push(Element const &element);

        /**
         * @brief Retrieves the total stride between consecutive vertices.
         *
         * @return Stride in bytes.
         */
        inline unsigned getStride() const { return m_stride; }

        /**
         * @brief Provides read-only access to the layout elements.
         *
         * @return Vector of Element descriptors.
         */
        inline std::vector<Element> const &getElements() const { return m_elements; }
    };

    /**
     * @brief Layout description for non-interleaved vertex attributes.
     *
     * Defines attribute elements including offset for each attribute.
     */
    class VertexBufferLayout
    {
    public: 
        /**
         * @brief Single element description in a non-interleaved layout.
         */
        struct Element {
            unsigned count;   ///< Number of components.
            GLenum   type;    ///< OpenGL type (e.g., GL_FLOAT).
            size_t   offset;  ///< Offset in bytes from buffer start.
        };

    private:
        std::vector<Element> m_elements; ///< Sequence of elements.

    public:
        /**
         * @brief Default constructor.
         */
        VertexBufferLayout() = default;

        /**
         * @brief Constructs layout from an initializer list of elements.
         *
         * @param elements List of Element descriptors.
         */
        VertexBufferLayout(std::initializer_list<Element> const &elements);

        /**
         * @brief Constructs layout from a vector of elements.
         *
         * @param elements Vector of Element descriptors.
         */
        VertexBufferLayout(std::vector<Element> const &elements);

        /**
         * @brief Destructor.
         */
        ~VertexBufferLayout() = default;

        /**
         * @brief Appends a new element to the layout.
         *
         * @param element The Element descriptor to add.
         */
        void push(Element const &element);

        /**
         * @brief Provides read-only access to the layout elements.
         *
         * @return Vector of Element descriptors.
         */
        inline std::vector<Element> const &getElements() const { return m_elements; }
    };

    /**
     * @brief Layout description for instanced vertex attributes.
     *
     * Defines attribute elements including divisor for instanced draws.
     */
    class InstancingVertexBufferLayout
    {
    public: 
        /**
         * @brief Single element description in an instanced layout.
         */
        struct Element {
            unsigned count;    ///< Number of components.
            GLenum   type;     ///< OpenGL type (e.g., GL_FLOAT).
            unsigned offset;   ///< Offset in bytes from buffer start.
            unsigned divisor;  ///< Attribute divisor for instancing.
        };

    private:
        std::vector<Element> m_elements; ///< Sequence of elements.

    public:
        /**
         * @brief Default constructor.
         */
        InstancingVertexBufferLayout() = default;

        /**
         * @brief Constructs layout from an initializer list of elements.
         *
         * @param elements List of Element descriptors.
         */
        InstancingVertexBufferLayout(std::initializer_list<Element> const &elements);

        /**
         * @brief Constructs layout from a vector of elements.
         *
         * @param elements Vector of Element descriptors.
         */
        InstancingVertexBufferLayout(std::vector<Element> const &elements);

        /**
         * @brief Destructor.
         */
        ~InstancingVertexBufferLayout() = default;

        /**
         * @brief Appends a new element to the layout.
         *
         * @param element The Element descriptor to add.
         */
        void push(Element const &element);

        /**
         * @brief Provides read-only access to the layout elements.
         *
         * @return Vector of Element descriptors.
         */
        inline std::vector<Element> const &getElements() const { return m_elements; }
    };

    /**
     * @brief Layout for interleaved instanced vertex attributes.
     *
     * Combines interleaved layout with per-element divisors.
     */
    class InterleavedInstancingVertexBufferLayout
    {
    public:
        /**
         * @brief Single element description in interleaved instanced layout.
         */
        struct Element {
            unsigned count;    ///< Number of components.
            unsigned type;     ///< OpenGL type (e.g., GL_FLOAT).
            unsigned divisor;  ///< Attribute divisor for instancing.
        };

    private:
        std::vector<Element> m_elements; ///< Sequence of elements.
        unsigned m_stride = 0;           ///< Total stride in bytes.

    public:
        /**
         * @brief Default constructor.
         */
        InterleavedInstancingVertexBufferLayout() = default;

        /**
         * @brief Constructs layout from an initializer list of elements.
         *
         * @param elements List of Element descriptors.
         */
        InterleavedInstancingVertexBufferLayout(std::initializer_list<Element> const &elements);

        /**
         * @brief Constructs layout from a vector of elements.
         *
         * @param elements Vector of Element descriptors.
         */
        InterleavedInstancingVertexBufferLayout(std::vector<Element> const &elements);

        /**
         * @brief Destructor.
         */
        ~InterleavedInstancingVertexBufferLayout() = default;

        /**
         * @brief Appends a new element to the layout and updates stride.
         *
         * @param element The Element descriptor to add.
         */
        void push(Element const &element);

        /**
         * @brief Provides read-only access to the layout elements.
         *
         * @return Vector of Element descriptors.
         */
        inline std::vector<Element> const &getElements() const { return m_elements; }

        /**
         * @brief Retrieves the total stride between consecutive vertices.
         *
         * @return Stride in bytes.
         */
        inline unsigned const &getStride() const { return m_stride; }
    };

    /**
     * @brief RAII wrapper for an OpenGL vertex array object (VAO).
     *
     * Manages creation, attribute setup for one or more VBOs, and deletion
     * of a GL_VERTEX_ARRAY. Inherits reference counting and deallocation
     * logic from Object.
     */
    class VertexArray : public Object
    {
    private:
        unsigned m_vertexAttribIndex = 0; ///< Next available attribute index.

    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no VAO is generated.
         */
        VertexArray() = default;

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL VAO if generated and if canDeallocate() returns true.
         */
        ~VertexArray();

        /**
         * @brief Constructs and configures a VAO with a single VBO and layout.
         *
         * Generates a VAO and, if the layout contains elements, calls
         * addBuffer(buffer, layout) to enable and set up vertex attributes.
         *
         * @tparam Layout_t Type of layout (one of the layout classes).
         * @param buffer The VertexBuffer to bind.
         * @param layout The layout describing attribute format.
         */
        template <typename Layout_t>
        explicit VertexArray(VertexBuffer const &buffer, Layout_t const &layout);

        /**
         * @brief Adds a VBO to this VAO using an interleaved layout.
         *
         * @param buffer The VertexBuffer to bind.
         * @param layout The InterleavedVertexBufferLayout descriptor.
         */
        void addBuffer(VertexBuffer const &buffer, InterleavedVertexBufferLayout const &layout);

        /**
         * @brief Adds a VBO to this VAO using a non-interleaved layout.
         *
         * @param buffer The VertexBuffer to bind.
         * @param layout The VertexBufferLayout descriptor.
         */
        void addBuffer(VertexBuffer const &buffer, VertexBufferLayout const &layout);

        /**
         * @brief Adds a VBO to this VAO using an interleaved instanced layout.
         *
         * @param buffer The VertexBuffer to bind.
         * @param layout The InterleavedInstancingVertexBufferLayout descriptor.
         */
        void addBuffer(VertexBuffer const &buffer, InterleavedInstancingVertexBufferLayout const &layout);

        /**
         * @brief Adds a VBO to this VAO using an instanced layout.
         *
         * @param buffer The VertexBuffer to bind.
         * @param layout The InstancingVertexBufferLayout descriptor.
         */
        void addBuffer(VertexBuffer const &buffer, InstancingVertexBufferLayout const &layout);

        /**
         * @brief Binds this VAO for subsequent draw calls.
         *
         * Calls glBindVertexArray(m_renderID).
         *
         * @param slot Ignored for VAOs; included for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

    template <typename Layout_t>
    inline VertexArray::VertexArray(VertexBuffer const &buffer, Layout_t const &layout)
    {
        glGenVertexArrays(1, &m_renderID);
        if(layout.getElements().size() != 0)
            addBuffer(buffer, layout);
    }

} // namespace ogl
