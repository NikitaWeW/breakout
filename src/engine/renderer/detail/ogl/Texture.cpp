#include "Texture.hpp"
#include "stb_image.h"
#include "engine/equirect.hpp"
#include <stdexcept>
#include <array>
#include <random>

ogl::Texture::Texture(unsigned) noexcept
{
    glCreateTextures(GL_TEXTURE_2D, 1, &m_renderID);
    
    glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
ogl::Texture::Texture(engine::Bitmap<float> const &bitmap, std::string_view type) noexcept : m_type(type)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &m_renderID);

    if(bitmap.getWidth() * bitmap.getHeight() > 10000) {
        glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format;
    switch (bitmap.getNumComponents())
    {
    case 1:
        format = GL_R16F;
        break;
    case 2:
        format = GL_RG16F;
        break;
    case 3:
        format = GL_RGB16F;
        break;
    case 4:
        format = GL_RGBA16F;
        break;
    default:
        format = GL_RGBA16F;
        break;
    }

    glTextureStorage2D(m_renderID, 1, format, bitmap.getWidth(), bitmap.getHeight());
}
ogl::Texture::~Texture()
{
    if(canDeallocate()) {
        glDeleteTextures(1, &m_renderID);
    }
}

ogl::TextureMS::TextureMS(unsigned) noexcept
{
    glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_renderID);
    
    glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
ogl::TextureMS::~TextureMS()
{
    if(canDeallocate()) {
        glDeleteTextures(1, &m_renderID);
    }
}

ogl::Cubemap::Cubemap(std::array<engine::Bitmap<float>, 6> const &bitmaps) noexcept
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_renderID);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(
        m_renderID, 
        1, 
        GL_RGB16F, 
        bitmaps[0].getWidth(), 
        bitmaps[0].getHeight()
    );

    for (unsigned i = 0; i < 6; ++i) {
        const void* sourceImage = bitmaps[i].getData();
        glTextureSubImage3D(
            m_renderID, 
            0,      // mipmap level
            0,      // xOffset
            0,      // yOffset
            i,      // zOffset (layer in the case of a cubemap)
            bitmaps[0].getWidth(), bitmaps[0].getHeight(),   // 2D image dimensions
            1,          // depth
            GL_RGB,     // format
            GL_FLOAT,   // data type
            sourceImage
        );
    }
}

ogl::Cubemap::Cubemap(unsigned) noexcept
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_renderID);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_renderID, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAX_LEVEL, 0);
    glTextureParameteri(m_renderID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_renderID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
