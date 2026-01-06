#pragma once

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <optional>
#include "engine/Resource/Resources.hpp"

// TODO: sort this mess out of Data.hpp

namespace engine
{
    struct Camera
    {
        glm::uvec2 size{0};
        glm::mat4 projMat{1.0f};
        glm::mat4 viewMat{1.0f};
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
    
    /// @brief An instance of a model. 
    /// An entity with an instance could contain its own material which will be used instead of the model material.
    struct Instance
    {
        ecs::entity e_model = 0;
    };

    struct Version
    {
    public:
        using value_t = uint32_t;
    private:
        value_t mValue = 1;
    public:
        inline value_t get() const  { return mValue; }
        inline value_t increment() { return ++mValue; }
        inline bool operator==(Version const &other) { return mValue == other.mValue; }
    };
} // namespace engine
