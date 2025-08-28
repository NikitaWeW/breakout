#pragma once
#include "entt.hpp"
#include "GLFW/glfw3.h"

#define RENDERER_ASSERT(x, msg) (assert((x) && (msg)))

namespace engine
{
    namespace renderer
    {
        struct window
        {
            GLFWwindow *glfwWindow;
        };

        void setup(entt::registry &reg);
        void render(entt::registry &reg);
    } // namespace renderer
} // namespace engine
