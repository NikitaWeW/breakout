#pragma once
#include "engine/renderer/renderer.hpp"
#include "detail/detail.hpp"

namespace engine
{
    class EngineRenderer : public IRenderer
    {
    public:
        /** \brief The renderer settings. */
        struct Context
        {
            // contains the engine::Camera component
            ecs::entity e_camera;
            size_t shadowMapSize = 1024;
            float shadowMapNearPlane = 0.01f;
            float shadowMapFarPlane = 100;
        };
    protected: // expose some of the implementation to be able to override
        Context m_context;
        void renderMain(ecs::registry &reg, detail::RendererData &data);
    public:
        EngineRenderer() = default;
        EngineRenderer(Context const &context);

        void setup(ecs::registry &reg) override;
        void processData(ecs::registry &reg) override;
        void draw(ecs::registry &reg) override;
    };
} // namespace engine
