#include "engine/Input/input.hpp"
#include "engine/Header/data.hpp"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ecs::registry &reg = *static_cast<ecs::registry *>(glfwGetWindowUserPointer(window));

    engine::input::KeyEvent event = {
        .window = window,
        .key = key,
        .scancode = scancode,
        .action = action,
        .mods = mods
    };

    for(ecs::entity e_listener : reg.view<engine::input::InputListener>())
    {
        auto &listener = reg.get<engine::input::InputListener>(e_listener);
        listener.keyEvents.emplace(event);
    }
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    ecs::registry &reg = *static_cast<ecs::registry *>(glfwGetWindowUserPointer(window));

    engine::input::CursorPosEvent event = {
        .window = window,
        .cursorPos = {(float) xpos, (float) ypos},
    };

    for(ecs::entity e_listener : reg.view<engine::input::InputListener>())
    {
        auto &listener = reg.get<engine::input::InputListener>(e_listener);
        if(listener.prevCursorPos.x != -1)
            event.cursorDeltaPos = event.cursorPos - listener.prevCursorPos;
        listener.prevCursorPos = event.cursorPos;
        listener.cursorPosEvents.emplace(event);
    }
}
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ecs::registry &reg = *static_cast<ecs::registry *>(glfwGetWindowUserPointer(window));

    engine::input::MouseButtonEvent event{
        .window = window,
        .button = button,
        .action = action,
        .mods = mods
    };

    for(ecs::entity e_listener : reg.view<engine::input::InputListener>())
    {
        auto &listener = reg.get<engine::input::InputListener>(e_listener);
        listener.mouseButtonEvents.emplace(event);
    }
}
static void scroll_callback(GLFWwindow* window, double, double yoffset)
{
    ecs::registry &reg = *static_cast<ecs::registry *>(glfwGetWindowUserPointer(window));

    engine::input::ScrollEvent event = {
        .window = window,
        .scroll = (float) yoffset
    };

    for(ecs::entity e_listener : reg.view<engine::input::InputListener>())
    {
        auto &listener = reg.get<engine::input::InputListener>(e_listener);
        listener.scrollEvents.emplace(event);
    }
}

void engine::input::setup(ecs::registry &reg)
{
    ENGINE_ASSERT_MSG(!reg.view<engine::Window>().empty(), "forgot to add a window?");
    for(ecs::entity e_window : reg.view<engine::Window>())
    {
        auto &window = reg.get<engine::Window>(e_window);
        
        glfwSetWindowUserPointer(window.glfwWindow, &reg);
        glfwSetKeyCallback(window.glfwWindow, key_callback);
        glfwSetCursorPosCallback(window.glfwWindow, cursor_position_callback);
        glfwSetMouseButtonCallback(window.glfwWindow, mouse_button_callback);
        glfwSetScrollCallback(window.glfwWindow, scroll_callback);
    }
}

void engine::input::update(ecs::registry &reg)
{
    ENGINE_ASSERT_MSG(!reg.view<engine::Window>().empty(), "forgot to add a window?");
    for(ecs::entity e_window : reg.view<engine::Window>())
    {
        auto &window = reg.get<engine::Window>(e_window);
        glfwGetWindowSize(window.glfwWindow, reinterpret_cast<int *>(&window.size.x), reinterpret_cast<int *>(&window.size.y));
    }
}