#pragma once
#include "utils/ECS.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/Framebuffer.hpp"
#include "opengl/Shader.hpp"
#include "glm/glm.hpp"
#include "opengl/IndexBuffer.hpp"
#include <optional>

namespace game
{
    struct Drawable
    {
        opengl::ShaderProgram *shader;
        opengl::VertexBuffer vb;
        opengl::VertexArray va;
        std::optional<opengl::IndexBuffer> ib;
        unsigned count;
        GLenum mode;
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
    struct Color
    {
        glm::vec4 color;
    };
    struct ModelMatrix
    {
        glm::mat4 modelMatrix;
    };

    class Renderer : public ecs::System
    {
    private:
        opengl::Texture m_notfound{"res/textures/notfound.png", false, true};
    public:
        void update(double deltatime);
    };
} // namespace game
