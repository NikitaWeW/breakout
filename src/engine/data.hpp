#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <string>
#include "engine/config.hpp"
#include "ecs.hpp"
#include "bitmap.hpp"

namespace engine
{
    constexpr unsigned MAX_BONES_PER_VERTEX = 4;

    struct Mesh
    {
        std::vector<glm::vec4> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec4> tangents;
        std::vector<unsigned>  indices;
        std::vector<std::array<int, MAX_BONES_PER_VERTEX>> boneIDs;
        std::vector<std::array<float, MAX_BONES_PER_VERTEX>> weights;
    };
    struct MaterialTextures
    {
        ecs::entity ambient;
        ecs::entity diffuse;
        ecs::entity specular;
        ecs::entity bump;
        ecs::entity displacement;
        ecs::entity alpha;
        ecs::entity reflection;
    };
    struct Material
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 transmittance;
        glm::vec3 emission;

        float shininess;
        float ior;
    };
    struct Texture
    {
        Bitmap<float> data;
        std::string_view type = "unknown";
        bool grayscale = false;
    };
    struct Model
    {
        Mesh mesh;
        MaterialTextures textures;
        Material material;
    };
} // namespace engine
