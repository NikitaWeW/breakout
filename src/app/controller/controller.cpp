#include "controller.hpp"
#include "engine/input/input.hpp"
#include "engine/physics/physics.hpp"
#include "engine/data.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/io.hpp"

ecs::entity controller::createCamera(ecs::registry &reg, ecs::entity window)
{
    auto e = reg.create<engine::Camera, controller::ControllableCamera, engine::physics::MoveIntent, engine::Velocity, engine::Position, engine::Orientation, engine::input::InputListener>();
    ENGINE_ASSERT(reg.has<engine::Window>(window));
    reg.get<ControllableCamera>(e).window = window;
    return e;
}

void controller::update(ecs::registry &reg)
{
    for(ecs::entity e_camera : reg.view<engine::Camera, controller::ControllableCamera, engine::physics::MoveIntent, engine::Velocity, engine::Position, engine::Orientation, engine::input::InputListener>())
    {
        auto &controllable = reg.get<controller::ControllableCamera>(e_camera);
        auto &camera = reg.get<engine::Camera>(e_camera);
        auto &listener = reg.get<engine::input::InputListener>(e_camera);
        auto &velocity = reg.get<engine::physics::MoveIntent>(e_camera);
        auto &orientation = reg.get<engine::Orientation>(e_camera);
        auto &window = reg.get<engine::Window>(controllable.window);

        glm::mat4 invViewMat = glm::inverse(camera.viewMat);
        glm::vec3 right   = invViewMat * glm::vec4{1, 0, 0, 0};
        glm::vec3 up      = invViewMat * glm::vec4{0, 1, 0, 0};
        glm::vec3 forward = invViewMat * glm::vec4{0, 0,-1, 0};

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        velocity = {glm::vec3{0, 0, 0}};
        if(controllable.locked)
        {
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_W) == GLFW_PRESS) velocity += forward;
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_S) == GLFW_PRESS) velocity -= forward;
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_D) == GLFW_PRESS) velocity += right;
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_A) == GLFW_PRESS) velocity -= right;
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_E) == GLFW_PRESS) velocity += up;
            if(glfwGetKey(window.glfwWindow, GLFW_KEY_Q) == GLFW_PRESS) velocity -= up;
        }
        if(velocity != glm::vec3{0})
            velocity = {glm::normalize(velocity)};
        velocity *= controllable.speed;

        for(; !listener.keyEvents.empty(); listener.keyEvents.pop())
        {
            auto const &event = listener.keyEvents.front();

            if(event.key == GLFW_KEY_ESCAPE && event.action == GLFW_PRESS)
            {
                controllable.locked = !controllable.locked;
                controllable.firstTimeMovingMouse = true;
            }
        }
        glfwSetInputMode(window.glfwWindow, GLFW_CURSOR, controllable.locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        for(; !listener.cursorPosEvents.empty(); listener.cursorPosEvents.pop())
        {
            auto const &event = listener.cursorPosEvents.front();
            glm::vec2 offset = event.cursorDeltaPos * controllable.sensitivity;

            if(!controllable.firstTimeMovingMouse && controllable.locked)
            {
                auto newOrientation = glm::normalize(
                    glm::angleAxis(glm::radians(offset.y), glm::vec3{1, 0, 0}) * 
                    orientation * 
                    glm::angleAxis(glm::radians(offset.x), glm::vec3{0, 1, 0})
                );
                if(glm::abs(glm::vec3(glm::inverse(glm::mat4_cast(newOrientation)) * glm::vec4{0,0,-1,0}).y) < 0.99)
                    orientation = {newOrientation};
            }
            controllable.firstTimeMovingMouse = false;
        }

        for(; !listener.scrollEvents.empty(); listener.scrollEvents.pop())
        {
            auto const &event = listener.scrollEvents.front();

            if(controllable.locked)
            {
                controllable.fov -= event.scroll * 0.5;
                controllable.fov = glm::clamp<float>(controllable.fov, 0.05, 45);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        camera.size = window.size;

        camera.projMat = glm::perspective<float>(glm::radians(controllable.fov), (float) camera.size.x / (float) camera.size.y, controllable.znear, controllable.zfar);
        camera.viewMat = glm::translate(glm::mat4_cast(glm::normalize(orientation)), -reg.get<engine::Position>(e_camera));
    }
}
