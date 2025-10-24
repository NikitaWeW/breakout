#pragma once
#include "engine/renderer/engine_renderer/engineRenderer.hpp"

class BoneDebugRenderer : public engine::EngineRenderer
{
private:
    struct BoneModel {};
    engine::Model m_boneModel;
protected:
    void renderBones(ecs::registry &reg, engine::detail::RendererData const &data, ecs::entity const &e_instance);
    void renderMain(ecs::registry &reg, engine::detail::RendererData &data) override;
public:
    BoneDebugRenderer() = default;
    BoneDebugRenderer(Context const &context, engine::Model const &boneModel);

    void setup(ecs::registry &reg) override;
};