#include "Texture.hpp"
#include "stb_image.h"

opengl::Texture::Texture(unsigned width, unsigned height, GLenum format, GLenum wrap, GLenum filter)
{
    glGenTextures(1, &m_renderID);
    bind();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

opengl::Texture::Texture(std::string const &filepath, bool flip, bool srgb, GLenum wrap, GLenum filter, std::string const &type) : type(type)
{
    stbi_set_flip_vertically_on_load(flip);
    int width = 0, height = 0;
    unsigned char *buffer = nullptr;
    buffer = stbi_load(filepath.c_str(), &width, &height, nullptr, 4);

    glGenTextures(1, &m_renderID);
    bind();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    stbi_image_free(buffer);
}

opengl::Texture::~Texture()
{
    if(canDeallocate()) {
        glDeleteTextures(1, &m_renderID);
    }
}

void opengl::Texture::bind(unsigned slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_renderID);
}