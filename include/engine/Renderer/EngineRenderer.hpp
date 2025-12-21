#pragma once
#include "engine/DSA/ECS.hpp"
#include "engine/Renderer/Renderer.hpp"
#include "engine/Header/Handle.hpp"

namespace engine
{
    // FIXME: not in use
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

    /// Prevent an instance from getting drawn.
    struct RendererExclude {};

    /// Tag to exclude this object from shadow mapping pass. Can be added to lights and instances.
    struct NoShadow {};

    /// @brief The renderer settings.
    struct EngineRendererConfig
    {
        /// contains the engine::Camera component.
        ecs::entity e_camera = INVALID_ENTITY;

        /// @brief Controls the number of frames given to redraw the entire shadow map atlas. 
        /// More frames increase performance, but the shadows might lag behind.
        unsigned MAX_SHADOW_MAP_FRAMES = 4;
    };
    class EngineRenderer : public IRenderer, public Handle<struct EngineRendererImpl>
    {
    public:
        /// @brief Construct an invalid renderer.
        EngineRenderer() = default;
        /// @brief Construct a valid renderer
        /// @param reg The registry to draw to.
        /// @param conf The config duh.
        EngineRenderer(Registry &reg, EngineRendererConfig conf);

        void processData();
        // FIXME: registry is unused for now.
        // TODO: Remove the per-renderer registry from the constructor and make renderer setup automatically for each registry.
        void draw(Registry &) override;
    };
} // namespace engine
