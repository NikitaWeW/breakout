#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <string>
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
        MODEL, MESH, MATERIAL,
        AUDIO, 
        DATA
    };

    struct Texture
    {
        Bitmap<float> data;
        bool grayscale = false;
        bool srgb = false;
        std::string path;
    };
    struct Material
    {
        struct Textures
        {
            // entity with the Texture component
            ecs::entity ambient;
            ecs::entity diffuse;
            ecs::entity specular;
            ecs::entity bump;
            ecs::entity displacement;
            ecs::entity alpha;
            ecs::entity reflection;
        } textures;
        struct Properties
        {
            glm::vec3 ambient;
            glm::vec3 diffuse;
            glm::vec3 specular;
            glm::vec3 transmittance;
            glm::vec3 emission;
    
            float shininess;
            float ior;
        } properties;
    };
    struct Mesh
    {
        std::vector<glm::vec4> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec4> tangents;
        std::vector<glm::vec4> boneIDs;
        std::vector<glm::vec4> weights;
        std::vector<unsigned> indices;
        Material material;
    };
    struct Model
    {
        Mesh mesh;
        std::string path;
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
} // namespace engine
