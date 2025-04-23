#pragma once
#include "game/Renderer.hpp" // for Drawable struct
#include "utils/ECS.hpp" // for entity from model construction
#include <vector>
#include "assimp/scene.h"

namespace model
{
    struct MeshData {
        std::vector<unsigned>  indices;
        std::vector<glm::vec4> positions;
        std::vector<glm::vec2> textureCoords;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec4> tangents;
    };
    struct Mesh {
        std::optional<MeshData> data;
        std::optional<game::Drawable> drawable;
        std::vector<opengl::Texture> textures;
    };
    enum ModelLoadFlags {
        NONE               = 0,
        LOAD_DATA          = 1 << 0,
        LOAD_DRAWABLE      = 1 << 1,
        FLIP_TEXTURES      = 1 << 2,
        FLIP_WINDING_ORDER = 1 << 3 
    };
    class Model {
    private:
        std::vector<Mesh> m_meshes;
        std::filesystem::path m_directory;
        std::vector<std::pair<std::string, opengl::Texture>> m_loadedTextures;
        
        void processNode(aiNode *node, int const &flags, aiScene const *scene);
        Mesh processMesh(aiMesh *aimesh, int const &flags, aiScene const *scene);
        void loadMaterialTextures(std::vector<opengl::Texture> &textures, aiMaterial *material, aiTextureType const type, std::string const &typeName, int const &flags);
    public:
        Model() = default;
        Model(std::filesystem::path const &filePath, int flags = NONE);
        ~Model() = default;

        inline bool isLoaded() const { return m_meshes.size() != 0; }
        inline std::vector<Mesh> const &getMeshes() const { return m_meshes; }
        inline std::vector<Mesh> &getMeshes() { return m_meshes; }
    };
    template <typename Components_t> inline ecs::Entity_t makeModelEntity(Model const &model)
    {
        assert(false && "not implimented");
        return 0;
    }
} // namespace model
