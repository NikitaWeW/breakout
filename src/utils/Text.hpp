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
            float advance;

        };
        struct Atlas {
            opengl::Texture texture;
            unsigned size;
            unsigned glyfCount;
            std::map<char, GlyphData> glyphs;
        };
    private:
        opengl::ShaderProgram textShader;
        opengl::VertexBuffer quadVBO;
        opengl::VertexArray quadVAO;
        opengl::ShaderProgram atlasShader;
        opengl::ShaderProgram blurShader;
        Atlas atlas;
    public:
        float spaceSize = 0.05f;

        Font() = default;
        ~Font() = default;
        Font(std::filesystem::path const &filepath, std::vector<wchar_t> const &chars, unsigned atlasSize = 2048);
        void drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec3 const &color = {1, 1, 1});
        inline Atlas const &getAtlas() const { return atlas; }
    };
    template <typename T> std::vector<T> charRange(T first, T last)
    {
        std::vector<T> result(last - first);
        std::iota(result.begin(), result.end(), first);
        return result;
    }
} // namespace text
