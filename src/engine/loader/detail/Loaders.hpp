#pragma once
#include "engine/loader/loader.hpp"
#include "assimp/scene.h"

namespace engine::detail
{
    class TextureLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path) override;
    };
    class ModelLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path) override;
    };

} // namespace engine::detail
