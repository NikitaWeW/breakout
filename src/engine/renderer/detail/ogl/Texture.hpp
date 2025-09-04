#pragma once
#include "Object.hpp"
#include <string>
#include <filesystem>
#include "glad/gl.h"
#include "engine/bitmap.hpp"
#include <array>

namespace ogl
{
    class Texture : public Object
    {
    private:
        std::string m_type;
    public:
        Texture() = default;
        explicit Texture(unsigned) noexcept;
        explicit Texture(engine::Bitmap<float> const &bitmap, std::string_view type = "") noexcept;
        ~Texture();

        inline std::string_view getType() const { return m_type; }
    };
    class TextureMS : public Object
    {
    private:
        std::string m_type;
    public:
        TextureMS() = default;
        explicit TextureMS(unsigned) noexcept;
        ~TextureMS();

        inline std::string_view getType() const { return m_type; }
    };
    class Cubemap : public Object
    {
    public:
        Cubemap() = default;
        explicit Cubemap(std::array<engine::Bitmap<float>, 6> const &bitmaps) noexcept;
        explicit Cubemap(unsigned) noexcept;
    };
} // namespace ogl
