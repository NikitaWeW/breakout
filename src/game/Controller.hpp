#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "utils/ECS.hpp"
#include "glm/glm.hpp"
#include <optional>

namespace game
{
    struct Window 
    {
        GLFWwindow *glfwwindow;
    };
    struct ControllableCamera
    {
        float speedUnitsPerSecond;
        float sensitivity;
        bool locked = true;

        // managed by CameraController system
        glm::vec<2, double> prevCursorPos;
        bool firstTimeMovingMouse = true;
    };

    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
    void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
    void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);


    class CameraController : public ecs::ISystem
    {
    public:
        struct KeyEvent {
            GLFWwindow *window; 
            int key; 
            int scancode; 
            int action; 
            int mods;
        };
        struct MouseEvent {
            GLFWwindow *window; 
            std::optional<double> xpos = {}, ypos = {};
            std::optional<int> button = {}, action = {}, mods = {};
            std::optional<double> xoffset = {}, yoffset = {};
        };
    private:
        std::queue<KeyEvent> m_keyQueue;
        std::queue<MouseEvent> m_mouseQueue;
    public:
        static CameraController *controllerCallbackUser; // glfw callbacks redirect here
        void pushEvent(KeyEvent const &event);
        void pushEvent(MouseEvent const &event);
        CameraController();
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
} // namespace game
