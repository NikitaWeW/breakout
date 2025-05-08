#pragma once
#include "game/Renderer.hpp" // for Drawable struct
#include "utils/ECS.hpp" // for entity from model construction
#include <vector>
#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include <list>
#include "glm/gtc/quaternion.hpp"

namespace model
{
    constexpr unsigned MAX_BONES_PER_VERTEX = 4;
    constexpr unsigned MAX_BONES = 100;

    struct MeshData 
    {
        std::vector<unsigned>  indices;
        
        // per vertex data
        std::vector<glm::vec4> positions;
        std::vector<glm::vec2> textureCoords;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec4> tangents;
        std::vector<std::array<int, MAX_BONES_PER_VERTEX>> boneIDs;
        std::vector<std::array<float, MAX_BONES_PER_VERTEX>> weights;
    };
    struct Mesh 
    {
        std::optional<MeshData> data;
        std::optional<game::Drawable> drawable;
        std::vector<opengl::Texture> textures;
    };
    enum LoadFlags 
    {
        NONE               = 0,
        LOAD_DATA          = 1 << 0,
        LOAD_DRAWABLE      = 1 << 1,
        FLIP_TEXTURES      = 1 << 2,
        FLIP_WINDING_ORDER = 1 << 3 
    };

    class Model 
    {
    private:
        // "memory friendly"
        std::vector<Mesh> m_meshes; 
        std::map<std::string, unsigned> m_boneMap;
        std::vector<glm::mat4> m_boneTransformations;
        std::vector<glm::mat4> m_tposeTransform;
        std::filesystem::path m_directory;
        std::vector<std::pair<std::string, opengl::Texture>> m_loadedTextures;
        glm::mat4 m_globalInverseTransorm;
        std::shared_ptr<Assimp::Importer> m_importer;
        aiScene const *m_scene;
        
        void processNode(aiNode const *node, int flags, aiScene const *scene);
        Mesh processMesh(aiMesh const *aimesh, int flags, aiScene const *scene);
    public:
        Model() = default;
        Model(std::filesystem::path const &filePath, int flags = NONE);
        ~Model() = default;

        std::vector<glm::mat4> const &getBoneTransformations(float animationTimeSeconds, aiAnimation const *animation);
        std::vector<glm::mat4> const &getBoneTransformations(aiAnimation const *animation, float animationTimeTicks);

        std::vector<glm::mat4> const &getBoneTransformations(aiAnimation const *first, aiAnimation const *second, float factor, float firstTimeTicks, float secondTimeTicks);
        std::vector<glm::mat4> const &getBoneTransformations(float firstTimeSeconds, float secondTimeSeconds, aiAnimation const *first, aiAnimation const *second, float factor);

        inline std::vector<Mesh> const &getMeshes() const { return m_meshes; }
        inline std::vector<Mesh> &getMeshes() { return m_meshes; }
        inline aiScene const *getScene() const { return m_scene; }
    };
} // namespace model
