#pragma once
#include "Object.hpp"
#include "glad/gl.h"
#include <cstddef>
#include <vector>

namespace opengl
{
    size_t getSizeOfGLType(GLenum type);
    class VertexBuffer : public Object 
    {
    private:
    public:
        VertexBuffer() = default;
        VertexBuffer(size_t size, GLenum usage = GL_DYNAMIC_DRAW);
        VertexBuffer(size_t size, void const *data, GLenum usage = GL_DYNAMIC_DRAW);
        ~VertexBuffer();    
        void bind(unsigned slot = 0) const noexcept;
    };

    class InterleavedVertexBufferLayout 
    {
    public:
        struct Element {
            unsigned count;
            unsigned type;
        };
    
    private:
        std::vector<Element> m_elements;
        unsigned m_stride = 0;
    public:
        InterleavedVertexBufferLayout() = default;
        ~InterleavedVertexBufferLayout() = default;
        void push(Element const &element);
        inline unsigned getStride() const { return m_stride; }
        inline std::vector<Element> const &getElements() const { return m_elements; }
    };
    class VertexBufferLayout
    {
    public: 
        struct Element {
            unsigned count;
            GLenum type;
            unsigned offset;
        };
    private:
        std::vector<Element> m_elements;
    public:
        VertexBufferLayout() = default;
        ~VertexBufferLayout() = default;
        void push(Element const &element);
        inline std::vector<Element> const &getElements() const { return m_elements; }
    };
    class InstancingVertexBufferLayout
    {
    public:
        struct Element {
            unsigned count;
            unsigned type;
            unsigned divisor;
        };
    private:
        std::vector<Element> m_elements;
        unsigned m_stride = 0;
    public:
        InstancingVertexBufferLayout() = default;
        ~InstancingVertexBufferLayout() = default;
        void push(Element const &element);
        inline std::vector<Element> const &getElements() const { return m_elements; }
        inline unsigned const &getStride() const { return m_stride; }
    };

    class VertexArray : public Object
    { // not a clean solution (thanks again, cherno)
    private:
        unsigned m_vertexAttribIndex = 0;
    public:
        VertexArray() = default;
        ~VertexArray();
        template <typename Layout_t> VertexArray(VertexBuffer const &buffer, Layout_t const &layout);
        void addBuffer(VertexBuffer const &buffer, InterleavedVertexBufferLayout const &layout);
        void addBuffer(VertexBuffer const &buffer, VertexBufferLayout const &layout);
        void addBuffer(VertexBuffer const &buffer, InstancingVertexBufferLayout const &layout);

        void bind(unsigned slot = 0) const noexcept;
    };
    template <typename Layout_t> inline VertexArray::VertexArray(VertexBuffer const &buffer, Layout_t const &layout) { 
        glGenVertexArrays(1, &m_renderID);
        addBuffer(buffer, layout); 
    }
} // namespace opengl
