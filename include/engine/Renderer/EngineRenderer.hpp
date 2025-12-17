#pragma once
#include "engine/Renderer/Renderer.hpp"

// FIXME: redo this renderer mess, make a better architecture/structure

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

    /// @brief Tag to render Instance as transparent. 
    /// Dont forget to add it to transparent objects or else they wont render.
    /// @see Instance
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
        /// @brief The renderer settings.
        struct Config
        {
            /// contains the engine::Camera component
            ecs::entity e_camera = 0;

            /// @brief Controls the number of frames given to redraw the entire shadow map atlas. 
            /// More frames increase performance, but the shadows might lag behind.
            unsigned MAX_SHADOW_MAP_FRAMES = 4;
        };
    // expose some of the implementation to be able to override it in the derivatives. 
    // FIXME: dont
    // TODO: pimpl this
    protected: 
        Config m_context;
        void renderMainInstance(Registry &reg, engine::renderer::ogl::Program const &shader, renderer::RendererData const &data, ecs::entity const &e_instance);
        void renderMain(Registry &reg, renderer::RendererData &data);

        void renderShadowMaps(Registry &reg, renderer::RendererData &data, unsigned toDraw);
        
        void setupPipeline(Registry &reg);

        void processModels(Registry &reg);
        void processTextures(Registry &reg);
        void processLights(Registry const &reg, renderer::RendererData &data);
    public:
        EngineRenderer() = default;
        EngineRenderer(Config const &context);

        /// Access the context that is used to setup registries. 
        /// Doesent affect registries that are already set up.
        inline Config const &context() const { return m_context; }
        inline Config &context() { return m_context; }

        void setup(Registry &reg) override;
        void processData(Registry &reg) override;
        void draw(Registry &reg) override;
    };
} // namespace engine
