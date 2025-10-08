#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include "engine/config.hpp"
#include "ecs.hpp"
#include "bitmap.hpp"
#include "glm/gtc/quaternion.hpp"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

namespace engine
{
    enum class DataType
    {
        MODEL, 
        MESH, 
        MATERIAL,
        AUDIO, 
        DATA,
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
        /**
         * Contains entities with the Texture component, 0 if not present.
         */
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
            glm::vec3 albedo;
            glm::vec3 specular;
            glm::vec3 transmittance;
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
        struct Primitives // TODO: rename to Geometry
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
        } primitives;
        Material material;
    };
    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Animation> animations;
        std::string path;
        struct Skeleton
        {
            std::vector<glm::mat4> inverseTposeTransform;
            std::vector<int> parents; // -1 if root
            std::unordered_map<std::string, unsigned> boneMap; // bone name to bone id
        } skeleton;
    };
    struct Camera
    {
        glm::uvec2 size{0};
        glm::mat4 viewMat{1.0f};
        glm::mat4 projMat{1.0f};
    };
    struct ModelMatrix 
    {
        glm::mat4 value;
    };

    struct Position : glm::vec3 {};
    struct Orientation : glm::quat {};
    struct Velocity : glm::vec3 {};

    // TODO: move it somewhere or do it better.
    /**
     * \brief The window representation. 
     */
    struct Window
    {
        GLFWwindow *glfwWindow;
        glm::uvec2 size{0};
    };
    
    struct Instance
    {
        ecs::entity e_model;
    };
} // namespace engine
