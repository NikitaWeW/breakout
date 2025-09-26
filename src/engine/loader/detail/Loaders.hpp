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
    struct LoaderData {
        engine::Material defaultMaterial;
        std::vector<std::unique_ptr<engine::loader::detail::ILoader>> loaders;
        std::unordered_map<std::string_view, engine::loader::detail::ILoader *> loaderMap;
    };

    void calculateMissingPrimitives(engine::Mesh &mesh);
} // namespace engine::loader::detail
