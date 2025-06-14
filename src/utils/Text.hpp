#pragma once
#include <map>
#include <numeric>
#include <filesystem>
#include <vector>
#include <string>

#include "opengl/Texture.hpp"
#include "opengl/Shader.hpp"
#include "opengl/Framebuffer.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/IndexBuffer.hpp"
#include "glm/glm.hpp"

namespace text
{
    /**
     * a font class that abstracts loading and drawing of a font (currently only [Artery Atlas Fonts](https://github.com/Chlumsky/artery-font-format) are supported)
     */
    class Font 
    {
    public:
        struct GlyphData {
            glm::vec2 offset;
            glm::vec2 size;
            float verticalOffset;
            float advance;
        };
        struct Atlas {
            opengl::Texture texture;
            std::map<wchar_t, GlyphData> glyphs;
            glm::vec2 dimensions;
        };
    private:
        opengl::ShaderProgram m_textShader;
        Atlas m_atlas;
        float m_newLineSize = 0;
        float m_spacing = 0;
        float m_spaceSize = 0;
        float m_pixelRange = 0;
        glm::vec2 m_atlasDimensions;
    public:
        Font() = default;
        ~Font() = default;
        Font(std::filesystem::path const &atlas, std::filesystem::path const &metadata);

        void drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec4 const &fgColor = {1, 1, 1, 1}, glm::vec4 const &bgColor = {0, 0, 0, 0}, glm::mat4 const &projectionMatrix = glm::mat4{1.0f});

        inline Atlas const &getAtlas() const { return m_atlas; }
    };
    template <typename T> std::vector<T> charRange(T first, T last)
    {
        std::vector<T> result(last - first);
        std::iota(result.begin(), result.end(), first);
        return result;
    }
} // namespace text
