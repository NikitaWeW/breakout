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

    class CameraController : public ecs::System
    {
    private:
    public:
        void update(double deltatime);
    };
} // namespace game
