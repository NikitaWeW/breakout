#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "engine/data.hpp"

namespace engine::loader::detail
{
    class ILoader
    {
    public:
        ILoader() = default;
        virtual ~ILoader() = default;
        virtual ecs::entity load(ecs::registry &reg, std::string_view path) = 0;
    };
    class TextureLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path) override;
    };
    class ObjModelLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path) override;
    };
    class GLTFModelLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path) override;
    };
} // namespace engine::loader::detail
