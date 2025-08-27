#pragma once

#include "Object.hpp"
#include <string>
#include <filesystem>
#include "glad/gl.h"

namespace ogl
{
    /**
     * @brief RAII wrapper for an OpenGL 2D texture object.
     *
     * Manages loading, sampling parameters, and deletion of an OpenGL
     * texture (GL_TEXTURE_2D). Inherits reference counting and deallocation
     * logic from Object.
     */
    class Texture : public Object
    {
    public:
        /**
         * @brief User-defined type descriptor for the texture.
         *
         * Can be used to label textures by purpose (e.g., "diffuse").
         */
        std::string type = "";

        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL texture is generated.
         */
        Texture() = default;

        /**
         * @brief Constructs a texture with specified sampling parameters.
         *
         * Generates and configures an OpenGL 2D texture with the given
         * minification filter, magnification filter, and wrapping mode.
         *
         * @param filtermin  Minification filter (e.g., GL_LINEAR).
         * @param filtermag  Magnification filter (e.g., GL_LINEAR).
         * @param wrap       Wrapping mode (default GL_CLAMP_TO_EDGE).
         */
        explicit Texture(GLenum filtermin,
                         GLenum filtermag,
                         GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;

        /**
         * @brief Loads a texture from an image file.
         *
         * Reads the image at filepath, optionally flips vertically, configures
         * sRGB sampling, and assigns a user-defined type. Reports whether the
         * image is grayscale via isGrayScalePtr.
         *
         * @param filepath         Path to the image file.
         * @param flip             Flip image vertically if true.
         * @param srgb             Use sRGB color space if true.
         * @param type             User-defined type string for the texture.
         * @param isGrayScalePtr   If non-null, set to true if the image is grayscale.
         */
        explicit Texture(std::filesystem::path const &filepath,
                         bool flip = false,
                         bool srgb = false,
                         std::string const &type = "",
                         bool *isGrayScalePtr = nullptr);

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL texture if generated and if canDeallocate()
         * returns true.
         */
        ~Texture();

        /**
         * @brief Binds this texture to the specified texture unit.
         *
         * Calls glBindTexture(GL_TEXTURE_2D, m_renderID).
         *
         * @param slot Texture unit to bind to (default 0).
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

    /**
     * @brief RAII wrapper for an OpenGL multisample texture object.
     *
     * Manages creation, sampling configuration, and deletion of an OpenGL
     * multisample texture (GL_TEXTURE_2D_MULTISAMPLE). Inherits reference
     * counting and deallocation logic from Object.
     */
    class TextureMS : public Object
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL texture is generated.
         */
        TextureMS() = default;

        /**
         * @brief Constructs a multisample texture with sampling parameters.
         *
         * Generates and configures an OpenGL multisample texture with the given
         * sampling filter and wrap mode.
         *
         * @param filter Minification and magnification filter.
         * @param wrap   Wrapping mode (default GL_CLAMP_TO_EDGE).
         */
        explicit TextureMS(GLenum filter,
                           GLenum wrap = GL_CLAMP_TO_EDGE) noexcept;

        /**
         * @brief Destructor.
         *
         * Deletes the multisample texture if generated and if canDeallocate()
         * returns true.
         */
        ~TextureMS();

        /**
         * @brief Binds this multisample texture to the specified unit.
         *
         * Calls glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_renderID).
         *
         * @param slot Texture unit to bind to (default 0).
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

    /**
     * @brief RAII wrapper for an OpenGL cubemap texture object.
     *
     * Manages loading from equirectangular projection, manual generation,
     * binding, and deletion of a GL_TEXTURE_CUBE_MAP. Inherits reference
     * counting and deallocation logic from Object.
     */
    class Cubemap : public Object
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves m_renderID at 0; no OpenGL cubemap is generated.
         */
        Cubemap() = default;

        /**
         * @brief Loads a cubemap from an equirectangular image file.
         *
         * Converts the given equirectangular projection into a cubemap,
         * optionally flipping vertically.
         *
         * @param filepath Path to the equirectangular image.
         * @param flip     Flip image vertically if true.
         */
        Cubemap(std::filesystem::path const &filepath, bool flip = false);

        /**
         * @brief Constructs and generates an empty cubemap.
         *
         * The dummy unsigned parameter exists to differentiate this overload.
         */
        explicit Cubemap(unsigned) noexcept;

        /**
         * @brief Binds this cubemap to the specified texture unit.
         *
         * Calls glBindTexture(GL_TEXTURE_CUBE_MAP, m_renderID).
         *
         * @param slot Texture unit to bind to (default 0).
         */
        void bind(unsigned slot = 0) const noexcept override;
    };

} // namespace ogl
