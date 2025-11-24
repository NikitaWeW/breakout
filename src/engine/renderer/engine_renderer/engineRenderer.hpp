#pragma once
#include "engine/renderer/renderer.hpp"

/// TODO: redo this renderer mess, make a better architecture/structure

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
    struct ShadowLight 
    {
        unsigned shadowMapSize = 1024;

        float nearPlane = 0.1;
        float farPlane = 100;
    };
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
        enum class Shape
        {
            RECTANGULAR, CIRCULAR
        } shape = Shape::RECTANGULAR;
    };

    /// \brief Tag to render Instance as transparent. 
    /// Dont forget to add it to transparent objects or else they wont render.
    /// \see Instance
    struct Transparent {};
    struct Skybox 
    {
        ecs::entity e_cubemap;
    };

    /// Prevent an instance from getting drawn by renderer.
    struct RendererExclude {};

    /// Tag to exclude this object from shadow mapping pass. Can be added to lights and instances.
    struct NoShadow {};

    class EngineRenderer : public IRenderer
    {
    public:
        /// \brief The renderer settings.
        struct Context
        {
            // contains the engine::Camera component
            ecs::entity e_camera = 0;
        };
    // expose some of the implementation to be able to override it in the derivatives.
    /// TODO: pimpl
    protected: 
        Context m_context;
        virtual void renderMainInstance(ecs::registry &reg, engine::renderer::ogl::Program const &shader, renderer::RendererData const &data, ecs::entity const &e_instance);
        virtual void renderMain(ecs::registry &reg, renderer::RendererData &data);

        virtual void renderShadowMaps(ecs::registry &reg, renderer::RendererData &data);
        
        virtual void setupPipeline(ecs::registry &reg);

        virtual void processModels(ecs::registry &reg);
        virtual void processMaterials(ecs::registry &reg);
        virtual void processTextures(ecs::registry &reg);
        virtual void processLights(ecs::registry const &reg, renderer::RendererData &data);
    public:
        EngineRenderer() = default;
        EngineRenderer(Context const &context);

        /// Access the context that is used to setup registries. 
        /// Doesent affect registries that are already set up.
        inline Context const &context() const { return m_context; }
        inline Context &context() { return m_context; }

        void setup(ecs::registry &reg) override;
        void processData(ecs::registry &reg) override;
        void draw(ecs::registry &reg) override;
    };
} // namespace engine
