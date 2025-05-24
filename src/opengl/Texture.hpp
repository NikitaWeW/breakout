#pragma once
#include "Object.hpp"
#include <string>
#include <filesystem>
#include "glad/gl.h"

namespace opengl
{
    class Texture : public Object
    {
    public:
        std::string type = "";
        Texture() = default;
        explicit Texture(GLenum filter, GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;
        explicit Texture(GLenum filtermin, GLenum filtermag, GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;
        explicit Texture(std::filesystem::path const &filepath, bool flip = false, bool srgb = false, std::string const &type = "");
        ~Texture();

        void bind(unsigned slot = 0) const noexcept;
    };
    class TextureMS : public Object
    {
    public:
        TextureMS() = default;
        explicit TextureMS(GLenum filter, GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;
        ~TextureMS();

        void bind(unsigned slot = 0) const noexcept;
    };
} // namespace opengl
