#pragma once
#include "engine/renderer/renderer.hpp"
namespace engine::renderer
{
    namespace ogl
    {
        struct Program;
    }
    struct RendererData;
}

namespace engine
{
    struct DynamicLight {};
    struct PointLight 
    {
        glm::vec3 color = {1, 1, 1};
        float intensity = 1;
    };
    struct DirectionalLight 
    {
        glm::vec3 color = {1, 1, 1};
    };
    struct SpotLight 
    {
        glm::vec3 color = {1, 1, 1};
        float innerConeAngle = 35;
        float outerConeAngle = 45;
        float intensity = 1;
    };
    struct AreaLight 
    {
        glm::vec3 color = {1, 1, 1};
        float intensity = 1;
        glm::vec2 size = {1, 1};
        enum class Type
        {
            RECTANGULAR, CIRCULAR
        } type = Type::RECTANGULAR;
    };

    /** 
     * \brief Tag to render Instance as transparent. 
     * Dont forget to add it to transparent objects or else they wont render.
     * \see Instance
     */
    struct Transparent {};

    class EngineRenderer : public IRenderer
    {
    public:
        /** \brief The renderer settings. */
        struct Context
        {
            // contains the engine::Camera component
            ecs::entity e_camera = 0;
            size_t shadowMapSize = 1024;
            float shadowMapNearPlane = 0.01f;
            float shadowMapFarPlane = 100;
            float shadowMapDirLightRange = 25;
        };
    // expose some of the implementation to be able to override it in the derivatives.
    protected: 
        Context m_context;
        virtual void renderMainInstance(ecs::registry &reg, engine::renderer::ogl::Program const &shader, renderer::RendererData const &data, ecs::entity const &e_instance);
        virtual void renderMain(ecs::registry &reg, renderer::RendererData &data);
        
        virtual void setupPipeline(ecs::registry &reg);

        virtual void processModels(ecs::registry &reg);
        virtual void processMaterials(ecs::registry &reg);
        virtual void processTextures(ecs::registry &reg);
        virtual void processLights(ecs::registry const &reg, renderer::RendererData &data);
    public:
        EngineRenderer() = default;
        EngineRenderer(Context const &context);

        void setup(ecs::registry &reg) override;
        void processData(ecs::registry &reg) override;
        void draw(ecs::registry &reg) override;
    };
} // namespace engine
