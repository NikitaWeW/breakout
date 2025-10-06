#pragma once
#include "engine/loader/loader.hpp"
#include "assimp/scene.h"
#include <optional>

namespace engine::detail
{
    class TextureLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
        ecs::entity load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags) override;
    };
    class ModelLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
    };

    std::optional<engine::Texture> loadTexture(std::string_view path, LoadingFlags flags);
    std::optional<engine::Texture> loadTexture(std::size_t size, void const *data, LoadingFlags flags);
} // namespace engine::detail
