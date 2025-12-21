#pragma once
#include "engine/DSA/Data.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "ogl.hpp"
#include <nicecs/ecs.hpp>
#include <unordered_map>

#define RENDERER_TRACE ENGINE_CORE_TRACE

namespace engine::renderer
{
    struct ProcessedTag {};

    // ================================================================
    // Models
    // ================================================================

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
        Material material;
        ogl::VAO vao;
        ogl::IBO ibo;
        GLenum mode = GL_TRIANGLES;

        unsigned count = 0;
    };
    struct Model
    {
        std::optional<renderer::Material> materialOverride;
        engine::Model::Skeleton skeleton;
        std::vector<Mesh> meshes;
        bool animated = false;
    };

    // ================================================================
    // Lighting
    // ================================================================

    struct AtlasLocation
    {
        /// In pixels
        glm::uvec2 pos{0};
        /// Size of a single cell of a shadow map in pixels.
        /// Regular ones consist of one cell, e.g. size x size
        /// Omnidirectional shadow maps consist of 6 cells , e.g. size x size x 6.
        unsigned size{0};
    };

    struct PointLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 position;
        float _pad1;
        AtlasLocation location;
        float farPlane;
    };
    struct DirLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 direction;
        float _pad1;
        AtlasLocation location;
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
        AtlasLocation location;
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

    class LightManager
    {
    private:
        struct Viewport
        {
            unsigned size;
        };
        // TODO: document these
        ecs::sparse_set<renderer::PointLight> mPointLights;
        ecs::sparse_set<renderer::DirLight>   mDirLights;
        ecs::sparse_set<renderer::SpotLight>  mSpotLights;
        ecs::sparse_set<renderer::DrawLight>  mDrawLights;

        ogl::SSBO mPointLightsSSBO;
        ogl::SSBO mDirLightsSSBO;
        ogl::SSBO mSpotLightsSSBO;
        ogl::SSBO mDrawLightsSSBO;

        ecs::sparse_set<Version> mVersions;
        ecs::sparse_set<Viewport> mViewports; // "memory efficient"
        
        struct Atlas
        {
            glm::uvec2 size{0};

            ogl::Texture texture;
            ogl::Framebuffer fbo;

            // One light may have multiple viewports (omnidirectional). Used for output only.
            std::vector<renderer::DrawLightViewport> viewports;

            size_t framesDrawn = 0;
        } mAtlas;

        bool mShouldUpdate = false;
    private:
        void processPointLight(Entity light);
        void processDirLight(Entity light);
        void processSpotLight(Entity light);
    
        void makeAtlas();
        void addLight(Entity light);
        void removeLight(Entity light);
        void updateLight(Entity light);
    public:
        /// @brief Adds or updates light if the versions mismatch.
        /// @param light An entity containing a Version, and lighting related components.
        void tryUpdateLight(Entity light);
        /// @brief Recompute atlas and update buffers if necessary.
        void apply();
        
        inline ogl::SSBO const &getPointLights() const { return mPointLightsSSBO; }
        inline ogl::SSBO const &getDirLights()   const { return mDirLightsSSBO;   }
        inline ogl::SSBO const &getSpotLights()  const { return mSpotLightsSSBO;  }
        inline ogl::SSBO const &getDrawLights()  const { return mDrawLightsSSBO;  }

        inline Atlas const &getAtlas() const { return mAtlas; }
    };


    // TODO: move this out of here or atleast move impl to .cpp
    inline ogl::Texture getTexture(engine::Registry const &reg, ecs::entity e_texture)
    {
        using namespace engine;
        if(e_texture == 0)
            return ogl::Texture{};
        ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedTag>(e_texture), "Forgot to call engine::EngineRenderer::processData?");

        return reg.get<ogl::Texture>(e_texture);
    }
    inline engine::renderer::Material convertMaterial(engine::Registry const &reg, engine::Material material)
    {
        return {
            .properties = material.properties,
            .textures = {
                .albedo    = getTexture(reg, material.textures.albedo),
                .normal    = getTexture(reg, material.textures.normal),
                .metallic  = getTexture(reg, material.textures.metallic),
                .roughness = getTexture(reg, material.textures.roughness) 
            }
        };
    }

} // namespace engine::renderer

struct engine::EngineRendererImpl
{
    // TODO: change the stype to mField
    struct Data
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
    
        renderer::LightManager mLightManager;

        EngineRendererConfig config;
        Registry *reg = nullptr;
    } data;

    void renderMainInstance(engine::ogl::Program const &shader, ecs::entity const &e_instance);
    void renderMain();

    void drawSM(size_t first, size_t count);
    void renderShadowMaps(unsigned toDraw);
    
    void setupPipeline();

    void processModels();
    void processTextures(); 

    void processData();
};

namespace ogl = engine::ogl;
