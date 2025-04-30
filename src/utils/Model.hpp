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

    class Model 
    {
    public:
        enum LoadFlags 
        {
            NONE               = 0,
            LOAD_DATA          = 1 << 0,
            LOAD_DRAWABLE      = 1 << 1,
            FLIP_TEXTURES      = 1 << 2,
            FLIP_WINDING_ORDER = 1 << 3 
        };
    private:
        std::vector<Mesh> m_meshes;
        std::map<std::string, unsigned> m_boneMap;
        std::vector<glm::mat4> m_boneTransformations;
        std::vector<glm::mat4> m_tposeTransform;
        std::filesystem::path m_directory;
        std::vector<std::pair<std::string, opengl::Texture>> m_loadedTextures;
        glm::mat4 m_globalInverseTransorm;
        std::shared_ptr<Assimp::Importer> m_importer;
        aiScene const *m_scene;
        
        void processNode(aiNode *node, int flags, aiScene const *scene);
        Mesh processMesh(aiMesh *aimesh, int flags, aiScene const *scene);
        void loadMaterialTextures(std::vector<opengl::Texture> &textures, aiMaterial *material, aiTextureType const type, std::string const &typeName, int flags);
    public:
        Model() = default;
        Model(std::filesystem::path const &filePath, int flags = NONE);
        ~Model() = default;

        std::vector<glm::mat4> const &getBoneTransformations(float animationTimeSeconds, aiAnimation const *animation);
        std::vector<glm::mat4> const &getBoneTransformations(aiAnimation const *animation, float animationTimeTicks);

        inline bool isLoaded() const { return m_meshes.size() != 0; }
        inline std::vector<Mesh> const &getMeshes() const { return m_meshes; }
        inline std::vector<Mesh> &getMeshes() { return m_meshes; }
        inline std::map<std::string, unsigned> const &getBoneMap() const { return m_boneMap; }
        inline std::map<std::string, unsigned> &getBoneMap() { return m_boneMap; }
        inline std::vector<glm::mat4> const &getBones() const { return m_boneTransformations; } // use getBoneTransformations
        inline std::vector<glm::mat4> &getBones() { return m_boneTransformations; }
        inline aiScene const *getScene() const { return m_scene; }
    };
} // namespace model
