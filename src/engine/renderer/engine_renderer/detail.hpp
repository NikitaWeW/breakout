#pragma once
#include "engine/Renderer/engineRenderer.hpp"
#include "ogl.hpp"
#include "engine/Animation/animation.hpp"

// FIXME: horrible mess

#define RENDERER_TRACE ENGINE_CORE_TRACE

namespace engine::renderer
{
    struct ProcessedModel {};
    struct ProcessedTexture {};
    struct ProcessedCubemap {};
    struct ProcessedMaterial {};

    struct Material
    {
        engine::Material::Properties properties;
        struct Textures
        {
            ogl::Texture albedo;
            ogl::Texture normal;
            ogl::Texture metallic;
            ogl::Texture roughness;
        } textures;
    };
    struct Mesh
    {
        ecs::entity e_material;
        ogl::VAO vao;
        ogl::IBO ibo;
        GLenum mode = GL_TRIANGLES;

        unsigned count = 0;
    };
    struct Model
    {
        engine::Skeleton skeleton;
        std::vector<Mesh> meshes;
        bool animated = false;
    };

    struct ShadowMapAtlas
    {
        struct Location
        {
            /// In pixels
            glm::uvec2 pos{0};
            /// Size of a single cell of a shadow map in pixels.
            /// Regular ones consist of one cell, e.g. size x size
            /// Omnidirectional shadow maps consist of 6 cells , e.g. size x size x 6.
            unsigned size{0};
        };
        struct Light
        {
            /// The type of the light, used to determine which light's location to update.
            enum { POINT, DIR, SPOT } type;
            /// An index of the draw light.
            /// Indexes ShadowMapAtlas::projMatrices and ShadowMapAtlas::viewMatrices
            size_t drawLightIndex;
            /// An index of the light used in the shading stage.
            /// Indexes the RendererData::pointLights, RendererData::dirLights, RendererData::spotLights.
            size_t lightIndex;
            /// In pixels. Used to set the size of the lights and stuff.
            unsigned size;
        };
        glm::uvec2 size{0};

        ogl::Texture texture;
        ogl::Framebuffer fbo;

        // TODO: distribute shadow map draws in a few frames.
        std::vector<Light> lights;
        std::vector<glm::mat4> viewMatrices;
        std::vector<glm::mat4> projMatrices;
    };
    struct PointLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 position;
        float _pad1;
        ShadowMapAtlas::Location location;
        float farPlane;
    };
    struct DirLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 direction;
        float _pad1;
        ShadowMapAtlas::Location location;
        float _pad2;
        glm::vec4 _pad3;
        glm::mat4 viewProj;
    };
    struct SpotLight
    {
        glm::vec3 color;     
        float _pad0;
        glm::vec3 position;  
        float _pad1;
        glm::vec3 direction; 
        float innerConeAngle;
        ShadowMapAtlas::Location location;
        float outerConeAngle;
        glm::mat4 viewProj;
    };
    struct DrawLight
    {
        glm::mat4 viewMat;
        glm::mat4 projMat;
    };
    struct DrawLightViewport
    {
        glm::vec2 pos;
        glm::vec2 size;
    };

    // FIXME: why
    struct RendererData
    {
        ogl::Framebuffer oitFBO;
        ogl::Texture oitAccumTexture;
        ogl::Texture oitRevealageTexture;

        ogl::Framebuffer mainFBO;
        ogl::Texture mainFBOColor;
        ogl::Renderbuffer mainFBORBO;

        glm::uvec2 prevCamSize{0};

        struct Shaders
        {
            ogl::Program screenShader;
            ogl::Program propShader;
            ogl::Program oitCompositeShader;
            ogl::Program skyboxShader;
            ogl::Program depthMapShader;
        } shaders;

        // The model loader should handle the default textures. This one is for edge cases.
        ogl::Texture defaultTexture;

        std::vector<renderer::PointLight> pointLights;
        std::vector<renderer::DirLight>   dirLights;
        std::vector<renderer::SpotLight>  spotLights;
        ogl::SSBO pointLightsSSBO;
        ogl::SSBO dirLightsSSBO;
        ogl::SSBO spotLightsSSBO;
        
        struct {
            std::vector<renderer::DrawLight> lights;
            ogl::SSBO lightsSSBO;
            std::vector<renderer::DrawLightViewport> viewports;
            ShadowMapAtlas atlas;
            size_t framesDrawn = 0;
            bool refresh = true;
        } SM;


        EngineRenderer::Config context;
    };
} // namespace engine::renderer
