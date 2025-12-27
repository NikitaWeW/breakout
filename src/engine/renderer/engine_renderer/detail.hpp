#pragma once
#include "engine/DSA/Data.hpp"
#include "engine/DSA/SparseSet.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "ogl.hpp"
#include <nicecs/ecs.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "engine/Logging/Logging.hpp"

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


    struct ShadowAtlas
    {
        glm::uvec2 size{0};

        ogl::Texture texture;
        ogl::Framebuffer fbo;
        ogl::SSBO drawLightsSSBO;

        /// One light may have multiple viewports and draw lights (omnidirectional). One-to-one mapping between draw lights and viewports.
        std::vector<renderer::DrawLight>         drawLights;
        std::vector<renderer::DrawLightViewport> viewports; 

        mutable size_t framesDrawn = 0;
        bool refreshed = true;
    };

    // TODO: Document it!
    // FIXME: A bit nicer than before, though I need to find a cleaner solution.
    class LightManager
    {
    private:
        // Is it even an optimization to maintain this many structures just to not recompute every light like a sane person?

        struct ShadowLight
        {
            unsigned size;
            glm::mat4 viewMat; // Doesent include any rotation for omnidirectional lights.
            glm::mat4 projMat;
        };

        SparseSet<renderer::PointLight> mPointLights;
        SparseSet<renderer::DirLight>   mDirLights;
        SparseSet<renderer::SpotLight>  mSpotLights;
        SparseSet<engine::ShadowLight>  mShadowLightsCache; /// Used to determine whether the shadow light was updated.

        std::unordered_set<ecs::entity> mThisFrameLights;
        std::unordered_set<ecs::entity> mLastFrameLights;
        
        ogl::SSBO mPointLightsSSBO;
        ogl::SSBO mDirLightsSSBO;
        ogl::SSBO mSpotLightsSSBO;

        SparseSet<Version> mVersions; // "memory efficient"
        SparseSet<ShadowLight> mShadowLights;
        
        ShadowAtlas mAtlas;
        bool mShouldUpdate = false;
        bool mViewportChanged = false;
        bool mDrawLightChanged = false;
    private:
        void processPointLight(Entity light);
        void processDirLight(Entity light);
        void processSpotLight(Entity light);
    
        void deleteLight(ecs::entity light);

        bool isOmnidirectional(ecs::entity e);
        bool isShadowLightChanged(ecs::entity e, engine::ShadowLight const &light);
        void populateBuffers();
        void makeAtlas();
        void makeDrawLights();
        // void sortViewports();
        void addLight(Entity light);
        void removeLight(Entity light);
        void updateLight(Entity light);
    public:
        /// @brief Adds or updates light if the versions mismatch.
        /// @param light An entity containing a Version, and lighting related components.
        void tryUpdateLight(Entity light);
        /// @brief Recompute atlas and update buffers if necessary.
        void apply();
        void setup();
        
        inline ogl::SSBO const &getPointLights() const { return mPointLightsSSBO; }
        inline ogl::SSBO const &getDirLights()   const { return mDirLightsSSBO;   }
        inline ogl::SSBO const &getSpotLights()  const { return mSpotLightsSSBO;  }

        inline size_t getNumPointLights() const { return mPointLights.size(); }
        inline size_t getNumDirLights()   const { return mDirLights.size();   }
        inline size_t getNumSpotLights()  const { return mSpotLights.size();  }

        inline ShadowAtlas const &getAtlas() const { return mAtlas; }
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
    // TODO: change the type to mField

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

    void renderMainInstance(engine::ogl::Program const &shader, ecs::entity const &e_instance);
    void renderMain();

    void drawSM(size_t first, size_t count);
    void renderShadowMaps(unsigned toDraw);
    
    void setupPipeline();

    void processModels();
    void processTextures(); 

    void processData();
    void draw();

    void setup();
};

namespace ogl = engine::ogl;
