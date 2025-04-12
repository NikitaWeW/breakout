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
    class Font 
    {
    public:
        struct GlyphData {
            glm::vec2 offset;
            glm::vec2 size;
            float verticalOffset;
        };
        struct Atlas {
            opengl::TextureMS texture;
            unsigned size;
            unsigned glyfCount;
            std::map<char, GlyphData> glyphs;
        };
    private:
        opengl::ShaderProgram textShader;
        opengl::VertexBuffer quadVBO;
        opengl::ShaderProgram atlasShader;
        opengl::ShaderProgram blurShader;
        Atlas atlas;
    public:
        float spaceSize = 0.05f;
        float newLineSize = 0.1f;
        float spacing = 0.005f;

        Font() = default;
        ~Font() = default;
        Font(std::filesystem::path const &filepath, std::vector<wchar_t> const &chars, unsigned atlasSize = 2048);
        void drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec4 const &color = {1, 1, 1, 1}, glm::mat4 const &projectionMatrix = glm::mat4{1.0f});
        inline void drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec3 const &color = {1, 1, 1}, glm::mat4 const &projectionMatrix = glm::mat4{1.0f}) { drawText(text, position, size, glm::vec4(color, 1), projectionMatrix); }
        inline Atlas const &getAtlas() const { return atlas; }
    };
    template <typename T> std::vector<T> charRange(T first, T last)
    {
        std::vector<T> result(last - first);
        std::iota(result.begin(), result.end(), first);
        return result;
    }
} // namespace text
