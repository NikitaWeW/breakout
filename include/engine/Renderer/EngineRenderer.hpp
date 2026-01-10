#pragma once
#include "engine/DSA/ECS.hpp"
#include "engine/Header/UID.hpp"
#include "engine/Renderer/Renderer.hpp"
#include "engine/Header/Handle.hpp"

namespace engine
{
    struct ShadowLight 
    {
        unsigned shadowMapSize = 1024;

        float nearPlane = 0.1;
        float farPlane = 100;

        inline bool operator==(ShadowLight const &other) 
        {
            return shadowMapSize == other.shadowMapSize &&
            glm::abs(nearPlane - other.nearPlane) <= 1e-6 &&
            glm::abs(farPlane - other.farPlane) <= 1e-6;
        }
        inline bool operator!=(ShadowLight const &other) { return !operator==(other); }
    };
    struct PointLight 
    {
        glm::vec3 color = {1, 1, 1};
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
    };
    struct AreaLight 
    {
        glm::vec3 color = {1, 1, 1};
        glm::vec2 size = {1, 1};
        // TODO: maybe SDF will describe it better?
        enum class Shape
        {
            RECTANGLE, CIRCLE, SPHERE
        } shape = Shape::RECTANGLE;
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
        /// Set to 1 to disable this feature.
        unsigned MAX_SHADOW_MAP_FRAMES = 2;
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
        ~EngineRenderer();

        void processData();
        void draw() override;

        void recompileShaders();
        void toggleDebugView();
    };
} // namespace engine
