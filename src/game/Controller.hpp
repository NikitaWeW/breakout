#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "utils/ECS.hpp"
#include "glm/glm.hpp"

namespace game
{
    struct ControllableCamera
    {
        GLFWwindow *window;
        float speedUnitsPerSecond;
        float sensitivity;
        bool locked = true;

        // managed by CameraController system
        glm::vec<2, double> prevCursorPos;
        bool firstTimeMovingMouse = true;
    };

    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    class CameraController : public ecs::System
    {
    public:
        static CameraController *controllerCallbackUser; // glfw callbacks redirect here
        void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        CameraController();
        void update(double deltatime);
    };
} // namespace game
