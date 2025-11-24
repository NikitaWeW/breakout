#pragma once
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "engine/core/ecs.hpp"
#include <queue>

/// TODO: better event and window system

namespace engine::input
{
    struct KeyEvent
    {
        GLFWwindow *window = nullptr;
        int key = 0;
        int scancode = 0;
        int action = 0;
        int mods = 0;
    };
    struct MouseButtonEvent
    {
        GLFWwindow *window = nullptr;
        int button = 0;
        int action = 0;
        int mods = 0;
    };
    struct CursorPosEvent
    {
        GLFWwindow *window = nullptr;
        glm::vec2 cursorPos;
        glm::vec2 cursorDeltaPos{0};
    };
    struct ScrollEvent
    {
        GLFWwindow *window = nullptr;
        float scroll = 0;
    };
    struct InputListener
    {
        glm::vec2 prevCursorPos{-1};
        std::queue<KeyEvent> keyEvents;
        std::queue<MouseButtonEvent> mouseButtonEvents;
        std::queue<CursorPosEvent> cursorPosEvents;
        std::queue<ScrollEvent> scrollEvents;
    };

    void setup(ecs::registry &reg);
    void update(ecs::registry &reg);
} // namespace engine::input
