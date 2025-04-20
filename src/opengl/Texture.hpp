#pragma once
#include "Object.hpp"
#include <string>
#include "glad/gl.h"

namespace opengl
{
    class Texture : public Object
    {
    public:
        std::string type = "";
        Texture() = default;
        explicit Texture(GLenum filter, GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;
        explicit Texture(std::string const &filepath, bool flip = false, bool srgb = false, GLenum filter = GL_NEAREST, GLenum wrap = GL_CLAMP_TO_EDGE, std::string const &type = "");
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
