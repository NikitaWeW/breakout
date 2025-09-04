#pragma once
#include "glm/glm.hpp"
#include <vector>

namespace engine
{
    /**
     * \brief A bitmap class, representing a 2d texture with the n number of components.
     * \tparam Format_t The underlying type.
     * \tparam components The number of glm::vec components used in the setPixel and getPixel operations.
     */
    template <typename Format_t = float, std::size_t components = 4>
    class Bitmap
    {
    private:
        unsigned m_width, m_height, m_numComponents;
        std::vector<Format_t> m_data;

        inline std::size_t getOffsetOf(unsigned x, unsigned y) const { return m_numComponents * (y * m_width + x); }
    public:
        /**
         * \brief Default constructor.
         */
        Bitmap() = default;
        /**
         * \brief Construct a valid bitmap.
         * \param width, height Dimensions of the bitmap
         * \param numComponents The number of channels / Format_t's per pixel
         * \param data The optional data. If not provided, the bitmap is filled with default values.
         */
        Bitmap(unsigned width, unsigned height, unsigned numComponents, Format_t const *data = nullptr);

        /**
         * \brief Sets the pixel at the (x; y) absolute coordinates to a specified value.
         * \param x The x coordinate in range of [0; width).
         * \param y The y coordinate in range of [0; height).
         * \param value The 4-component value. Only first numComponents will be set.
         */
        void setPixel(unsigned x, unsigned y, glm::vec<components, Format_t> const &value);
        /**
         * \brief Gets the pixel at (x, y).
         * \param x The x coordinate in range of [0; width).
         * \param y The y coordinate in range of [0; height).
         * \return The value at the (x; y) absolute coordinates. Only first numComponents will be filled, the rest will be filled with 0.
         */
        glm::vec<components, Format_t> getPixel(unsigned x, unsigned y) const;

        /**
         * \return The width of the bitmap.
         */
        unsigned getWidth() const;
        /**
         * \return The height of the bitmap.
         */
        unsigned getHeight() const;
        /**
         * \return The number of components in the bitmap.
         */
        unsigned getNumComponents() const;
        /**
         * \return glm::vec2{width, height}.
         */
        glm::vec2 getDimensions() const;
        /**
         * \return The data of the bitmap.
         */
        Format_t const *getData() const;
        /**
         * \copydoc getData.
         */
        Format_t *getData();
    };

} // namespace engine

/*! \cond Doxygen_Suppress */
template <typename Format_t, size_t components>
inline engine::Bitmap<Format_t, components>::Bitmap(unsigned width, unsigned height, unsigned numComponents, Format_t const *src) : m_width(width), m_height(height), m_numComponents(numComponents)
{
    EQUIRECT_ASSERT(m_numComponents <= 4, "Components > 4 is yet not supported!");
    m_data.resize(width * height * numComponents);
    if(src) {
        std::copy(src, src + m_data.size(), m_data.begin());
    }
}
template <typename Format_t, size_t components>
inline void engine::Bitmap<Format_t, components>::setPixel(unsigned x, unsigned y, glm::vec<components, Format_t> const &value)
{
    EQUIRECT_ASSERT(x < m_width && y < m_height, "x or y is out of range!");
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    Format_t *data = m_data.data();
    size_t offset = getOffsetOf(x, y);
    if (m_numComponents > 0) data[offset + 0] = value.x;
    if (m_numComponents > 1) data[offset + 1] = value.y;
    if (m_numComponents > 2) data[offset + 2] = value.z;
    if (m_numComponents > 3) data[offset + 3] = value.w;
}
template <typename Format_t, size_t components>
inline glm::vec<components, Format_t> engine::Bitmap<Format_t, components>::getPixel(unsigned x, unsigned y) const
{
    EQUIRECT_ASSERT(x < m_width && y < m_height, "x or y is out of range!");
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    Format_t const *data = m_data.data();
    size_t offset = getOffsetOf(x, y);
    return glm::vec4(
        m_numComponents > 0 ? data[offset + 0] : 0.0f,
        m_numComponents > 1 ? data[offset + 1] : 0.0f,
        m_numComponents > 2 ? data[offset + 2] : 0.0f,
        m_numComponents > 3 ? data[offset + 3] : 0.0f
    );
}
template <typename Format_t, size_t components> 
inline unsigned engine::Bitmap<Format_t, components>::getWidth() const 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return m_width; 
}
template <typename Format_t, size_t components> 
inline unsigned engine::Bitmap<Format_t, components>::getHeight() const 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return m_height; 
}
template <typename Format_t, size_t components> 
inline unsigned engine::Bitmap<Format_t, components>::getNumComponents() const 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return m_numComponents; 
}
template <typename Format_t, size_t components> 
inline glm::vec2 engine::Bitmap<Format_t, components>::getDimensions() const 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return glm::vec2{getWidth(), getHeight()}; 
}
template <typename Format_t, size_t components> 
inline Format_t const *engine::Bitmap<Format_t, components>::getData() const 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return m_data.data(); 
}
template <typename Format_t, size_t components> 
inline Format_t *engine::Bitmap<Format_t, components>::getData() 
{ 
    EQUIRECT_ASSERT(m_data.size() == m_height * m_width * m_numComponents, "Bitmap not initialized!");
    return m_data.data(); 
}
/*! \endcond */
