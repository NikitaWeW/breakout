#pragma once
#include "utils/ECS.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/Framebuffer.hpp"
#include "opengl/Shader.hpp"
#include "glm/glm.hpp"
#include "opengl/IndexBuffer.hpp"
#include "utils/Text.hpp"
#include "opengl/ShaderStorage.hpp"

#include <optional>

namespace game
{
    // The number of maximum lights per light type
    constexpr size_t MAX_LIGHTS = 5;
    constexpr size_t SHADOW_MAP_SIZE = 1024;
    constexpr float SHADOW_MAP_ZNEAR = 0.01f;
    constexpr float SHADOW_MAP_ZFAR = 100;

    struct Drawable
    {
        opengl::VertexBuffer vb;
        opengl::VertexArray va;
        std::optional<opengl::IndexBuffer> ib;
        unsigned count;
        GLenum mode = GL_TRIANGLES;
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
    struct RenderTarget
    {
        opengl::Framebuffer oitFBO{0}; // 0 -- dummy argument, constructor generates ogl object. TODO: find a better way avoiding dummy arguments
        opengl::Texture oitAccumTexture{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER};
        opengl::Texture oitRevelageTexture{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER};

        opengl::Framebuffer mainFBO{0};
        opengl::Texture mainFBOColor{GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER};
        opengl::Renderbuffer mainFBORBO{0};

        glm::vec4 clearColor{0, 0, 0, 1};
        unsigned outputFBOid = 0;
        int prevWidth = -1;
        int prevHeight = -1;
    };
    
    // marker components
    struct PerspectiveProjection {}; 
    struct Transparent {};
    struct SemiTransparent {};
    struct Skybox {};
    struct CastsShadow {};

    struct Color
    {
        glm::vec4 color;
    };
    struct ModelMatrix
    {
        glm::mat4 modelMatrix;
    };
    
    struct Text
    {
        text::Font *font;
        std::string text;
        glm::vec2 position; // 
        float size;
        glm::vec4 fgColor;
        glm::vec4 bgColor;
        std::optional<glm::mat4> matrix;
    };

    struct RepeatTexture
    {
        unsigned num = 1;
    };
    struct MaterialProperties 
    {
        float shininess;
    };
    
    struct LightUBO
    {
        opengl::UniformBuffer ubo;
    };
    struct LightSamplers
    {
        std::map<size_t, opengl::Cubemap *> pointLightSamplers;
        std::map<size_t, opengl::Texture *> dirLightSamplers;
        std::map<size_t, opengl::Texture *> spotLightSamplers;
    };
    // TODO: rename it into something more intuitive
    struct ShadowCaster {
        std::optional<opengl::Texture> regularShadowMap;
        std::optional<opengl::Cubemap> omnidirectionalShadowMap;
        opengl::Framebuffer fbo;
        glm::mat4 projMat;
        glm::mat4 viewMat;
        float farPlane;
        float nearPlane;
    };
    
    struct Light {
        glm::vec3 color;
    };
    struct PointLight {
        float attenuation;
    };
    struct DirectionalLight {};
    struct SpotLight {
        float innerConeAngle;
        float outerConeAngle;
        float attenuation;
    };
    // TODO: implement
    struct AreaLight {
        float attenuation;
        glm::vec2 size;
    };

    class Renderer : public ecs::ISystem
    {
    private:
        std::map<std::string, opengl::Texture> m_defaultTextures{
            {"", opengl::Texture{"res/textures/white.png", false, false}},
            {"diffuse", opengl::Texture{"res/textures/notfound.png", false, true}},
            {"normal", opengl::Texture{"res/textures/blue.png", false, false}}
        };
        opengl::Cubemap m_defaultCubemap{"res/textures/white.png"};

        opengl::ShaderProgram m_screenShader{"shaders/hdrImage"};
        opengl::ShaderProgram m_propShader{"shaders/prop"};
        opengl::ShaderProgram m_oitShader{"shaders/oitTransparent"};
        opengl::ShaderProgram m_oitCompositeShader{"shaders/oitComposite"};
        opengl::ShaderProgram m_skyboxShader{"shaders/skybox"};
        opengl::ShaderProgram m_depthMapShader{"shaders/depthMapOpaque"};
        opengl::ShaderProgram m_depthMapOmnidirectionalShader{"shaders/depthMapOmnidirectionalOpaque"};

        // easier access
        std::optional<opengl::UniformBuffer *> m_lightsUBO;
        std::optional<LightSamplers *> m_lightSamplers;

        void renderMain(std::set<ecs::Entity_t> const &entities, game::Camera &camera, game::RenderTarget &rtarget);
        void renderShadowMaps(std::set<ecs::Entity_t> const &entities, game::Camera &camera, game::RenderTarget &rtarget);
        void drawModel(ecs::Entity_t const &entity, opengl::ShaderProgram const &shader) const;
        void setCommonUniforms(opengl::ShaderProgram const &shader, game::Camera const &camera, glm::vec3 const &cameraPosition);

    public:
        Renderer() = default;
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
    class LightUpdater : public ecs::ISystem
    {
    public:
        // lighting shader side light structs
        struct ShaderPointLight
        {
            glm::vec3 color;
            float attenuation;
            glm::vec3 position;
            float farPlane;
        };
        struct ShaderDirLight
        {
            glm::vec3 direction;
            float _pad0;
            glm::vec3 color;
            float _pad1;
            glm::mat4 viewProj;
        };
        struct ShaderSpotLight
        {
            glm::vec3 position;
            float innerConeAngle;
            glm::vec3 direction;
            float outerConeAngle;
            glm::vec3 _pad0;
            float attenuation;
            glm::vec3 color;
            float _pad1;
            glm::mat4 viewProj;
        };
        struct LightStorage
        {
            unsigned numPointLights;
            glm::vec3 _pad0;
            std::array<ShaderPointLight, MAX_LIGHTS> pointLights;
            unsigned numDirLights;
            glm::vec3 _pad1;
            std::array<ShaderDirLight, MAX_LIGHTS> dirLights;
            unsigned numSpotLights;
            glm::vec3 _pad2;
            std::array<ShaderSpotLight, MAX_LIGHTS> spotLights;
        };
    public:
        LightUpdater();
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
}

