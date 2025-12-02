#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

#include "glm/glm.hpp"
#include "engine/core/ecs.hpp"
#include "bitmap.hpp"
#include "glm/gtc/quaternion.hpp"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

// TODO: sort this data.hpp mess

namespace engine
{
    enum class DataType
    {
        MODEL, 
        MESH, 
        CUBEMAP, 
        MATERIAL,
        AUDIO, 
        BUFFER,
        TEXTURE2D
    };

    struct Texture
    {
        Bitmap<float> data;
        bool srgb = false;
        std::string path;
    };
    struct Material
    {
        /// Contains entities with the Texture component, 0 if not present.
        struct Textures
        {
            ecs::entity albedo = 0;
            ecs::entity metallic = 0;
            ecs::entity roughness = 0;
            ecs::entity ambient = 0;
            ecs::entity normal = 0;
            ecs::entity displacement = 0;
            ecs::entity alpha = 0;
        } textures;
        struct Properties
        {
            glm::vec3 ambient;
            glm::vec4 albedo;
            glm::vec3 specular;
            glm::vec3 emission;
    
            float shininess;
            float metallic;
            float ior;
        } properties;
    };
    struct Animation
    {
        struct PositionKey
        {
            glm::vec3 value;
            float timeTicks;
        };
        struct OrientationKey
        {
            glm::quat value;
            float timeTicks;
        };
        struct ScaleKey
        {
            glm::vec3 value;
            float timeTicks;
        };
        struct Keyframes
        {
            std::vector<PositionKey   > positions;
            std::vector<OrientationKey> orientations;
            std::vector<ScaleKey      > scales;
        };

        std::vector<Keyframes> bones;
        std::string name = "";
        float durationTicks = 0;
        float ticksPerSecond = 0;
    };
    struct Mesh
    {
        struct Geometry
        {
            // guaranteed
            std::vector<glm::vec4> positions;
            std::vector<glm::vec2> texCoords;
            std::vector<glm::vec4> normals;
            std::vector<glm::vec4> tangents;
            std::vector<unsigned> indices;

            // optional
            // hope 4 bones per vertex would be enough
            std::vector<glm::vec4> boneIDs;
            std::vector<glm::vec4> weights;
        } geometry;
        // Must have engine::Material
        ecs::entity e_material;
    };
    struct Skeleton
    {
        glm::mat4 globalInverseTransform;
        std::vector<glm::mat4> bindTransform;
        std::vector<glm::mat4> nodeTransform;
        std::vector<int> parents; // -1 if root
        std::unordered_map<std::string, unsigned> boneMap; // bone name to bone id
    };
    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Animation> animations;
        std::string path;
        Skeleton skeleton;
    };
    struct Camera
    {
        glm::uvec2 size{0};
        glm::mat4 projMat{1.0f};
        glm::mat4 viewMat{1.0f};
    };

    struct Cubemap
    {
        std::array<Bitmap<float>, 6> faces;
        std::string path;
    };

    struct Transform
    {
        glm::vec3 position{0};
        glm::quat orientation{1, 0, 0, 0};
        glm::vec3 scale{1};
        inline glm::mat4 getMat() const {
            return glm::translate(glm::mat4{1.0f}, position) * glm::mat4_cast(orientation) * glm::scale(glm::mat4{1.0f}, scale);
        };
    };

    // TODO: move it somewhere or do it better.
    struct Window
    {
        GLFWwindow *glfwWindow;
        glm::uvec2 size{0};
    };
    
    struct Instance
    {
        ecs::entity e_model = 0;
        /// Overrides the material contained in the Model of the e_material if not 0. contains engine::Material component.
        // TODO: dont store materials in entities, store them directly
        ecs::entity e_material = 0;
    };
} // namespace engine
