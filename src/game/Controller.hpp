#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "utils/ECS.hpp"
#include "glm/glm.hpp"

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

    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    class CameraController : public ecs::ISystem
    {
    private:
        struct KeyEvent {
            GLFWwindow *window; 
            int key; 
            int scancode; 
            int action; 
            int mods;
        };
        std::queue<KeyEvent> m_keyQueue;
    public:
        static CameraController *controllerCallbackUser; // glfw callbacks redirect here
        void pushKeyEvent(KeyEvent const &event);
        CameraController();
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };
} // namespace game
