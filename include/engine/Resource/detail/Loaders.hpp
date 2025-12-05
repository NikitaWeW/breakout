#pragma once
#include "engine/Resource/loader.hpp"
#include "assimp/scene.h"
#include <optional>

namespace engine::loader
{
    class TextureLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
        ecs::entity load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags) override;
    };
    class CubemapLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
        ecs::entity load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags) override;
    };
    class ModelLoader : public ILoader
    {
    private:
        aiScene const *currentScene;
        engine::Model *currentModel;
        ecs::registry *currentRegistry;
        engine::LoadingFlags currentFlags;
    private:
        void loadMaterialTexture(aiMaterial const *material, aiTextureType const type, ecs::entity &out);
        engine::Material convertMaterial(aiMaterial const *aimaterial, engine::Material::Properties const &defaultProperties);
        engine::Mesh processMesh(aiMesh const *aimesh, glm::mat4 const &transform);
        void processNodeMeshes(aiNode const *node, glm::mat4 parentTransform);
        engine::Animation processAnimation(aiAnimation const *animation);

        ecs::entity load();
    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
        ecs::entity load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags) override;
    };
} // namespace engine::loader
