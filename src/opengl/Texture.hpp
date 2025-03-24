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
        Texture(unsigned width, unsigned height, GLenum format = GL_RGBA, GLenum wrap = GL_CLAMP_TO_EDGE, GLenum filter = GL_NEAREST);
        Texture(std::string const &filepath, bool flip = false, bool srgb = false, GLenum wrap = GL_CLAMP_TO_EDGE, GLenum filter = GL_NEAREST, std::string const &type = "");
        ~Texture();

        void bind(unsigned slot = 0) const;
    };
} // namespace opengl
