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
    private:
        aiScene const *currentScene;
        engine::Model *currentModel;
        ecs::registry *currentRegistry;
        engine::LoadingFlags currentFlags;
    private:
        void calculateMissingPrimitives(engine::Mesh &mesh);
        void optimizeMesh(engine::Mesh &mesh);
        void moveMesh(engine::Mesh::Geometry &primitives, glm::mat4 const &mat);
        void loadMaterialTexture(aiMaterial const *material, aiTextureType const type, ecs::entity &out);
        void extractBoneData(aiMesh const *aimesh, engine::Mesh &mesh);
        void processAnimationNode(engine::Animation &result, aiAnimation const *animation, aiNode const *node);
        void processNodeMeshes(aiNode const *node, glm::mat4 parentTransform);
        void extractVertexData(aiMesh const *aimesh, engine::Mesh &mesh);
        engine::Material getDefaultMaterial();
        engine::Material convertMaterial(aiMaterial const *aimaterial, engine::Material::Properties const &defaultProperties);
        engine::Mesh processMesh(aiMesh const *aimesh, glm::mat4 const &transform);
        engine::Animation processAnimation(aiAnimation const *animation);
        void calculateParent(aiNode const *node, int parent, glm::mat4 parentTransform);
        ecs::entity fromRawAssimpTexture(aiTexture const *texture);

    public:
        ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) override;
    };

    std::optional<engine::Texture> loadTexture(std::string_view path, LoadingFlags flags);
    std::optional<engine::Texture> loadTexture(std::size_t size, void const *data, LoadingFlags flags);
} // namespace engine::detail
