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
        opengl::ShaderProgram *shader;
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
        opengl::Framebuffer OIT_transparentFBO; // TODO: omit opaqueFBO, make the only OIT_FBO 
        opengl::Framebuffer OIT_opaqueFBO;
        opengl::Texture OIT_accumTexture {GL_LINEAR};
        opengl::Texture OIT_revealTexture{GL_LINEAR};
        opengl::Texture OIT_opaqueTexture{GL_LINEAR};
        opengl::Texture OIT_depthTexture {GL_NEAREST};
    };

    class Renderer : public ecs::ISystem
    {
    private:
        opengl::Texture m_notfound{"res/textures/notfound.png", false, true};
        opengl::Texture m_whiteTexture{"res/textures/white.png", false, false};
        opengl::ShaderProgram m_transparentShader{"shaders/transparent"};
        opengl::ShaderProgram m_oitCompositeShader{"shaders/oitComposite"};
        opengl::ShaderProgram m_hdrShader{"shaders/hdrImage"};
    public:
        Renderer() = default;
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
} // namespace game
