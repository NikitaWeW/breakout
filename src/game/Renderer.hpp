#pragma once
#include "utils/ECS.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/Framebuffer.hpp"
#include "opengl/Shader.hpp"
#include "glm/glm.hpp"
#include "opengl/IndexBuffer.hpp"
#include "utils/Text.hpp"

#include <optional>

namespace game
{
    struct Drawable
    {
        opengl::VertexBuffer vb;
        opengl::VertexArray va;
        std::optional<opengl::IndexBuffer> ib;
        unsigned count;
        GLenum mode = GL_TRIANGLES;
    };
    struct Text
    {
        text::Font *font;
        std::string text;
        glm::vec2 position;
        float size;
        std::optional<glm::mat4> matrix;
    };
    struct Camera
    {
        float zfar = 100;
        float znear = 0.01f;
        float fov = 45;
        int width = 0; 
        int height = 0;

        // calculated by renderer system
        glm::mat4 viewMat;
        glm::mat4 projMat;
    };
    struct PerspectiveProjection {}; // marker component
    struct Transparent {}; // also a marker component
    struct Color
    {
        glm::vec4 color;
    };
    struct ModelMatrix
    {
        glm::mat4 modelMatrix;
    };
    struct RenderTarget
    {
        glm::vec4 clearColor{0, 0, 0, 1};
        unsigned mainFBOid = 0;
        int prevWidth = -1, prevHeight = -1;
    };

    class Renderer : public ecs::ISystem
    {
    private:
        std::map<std::string, opengl::Texture> m_defaultTextures{
            {"", opengl::Texture{"res/textures/white.png", false, false}},
            {"diffuse", opengl::Texture{"res/textures/notfound.png", false, true}},
            {"normal", opengl::Texture{"res/textures/blue.png", false, false}}
        };
        opengl::ShaderProgram m_screenShader{"shaders/hdrImage"};
        opengl::ShaderProgram m_defaultShader{"shaders/plainColor"};

        void render(std::set<ecs::Entity_t> const &entities, double deltatime, game::Camera &camera, game::RenderTarget &rtarget);
    public:
        Renderer() = default;
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
}
