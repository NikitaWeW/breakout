#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "utils/ECS.hpp"
#include "glm/glm.hpp"

namespace game
{
    class CameraController : public ecs::System
    {
    private:
        glm::vec<2, double> m_prevCursorPos;
        bool firstTimeMovingMouse = true;
    public:
        GLFWwindow *window;
        void update(double deltatime);
    };
} // namespace game
