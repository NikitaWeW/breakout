#include "controller.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/io.hpp"

#define CAMERA_COMPONENTS engine::Camera, engine::ModelMatrix, engine::ModelMatrixAssemblerExclude, Controller::ControllableCamera, engine::Velocity, engine::Position, engine::Orientation, engine::input::InputListener

ecs::entity Controller::createCamera(ecs::registry &reg, ecs::entity window)
{
    auto e = reg.create<CAMERA_COMPONENTS>();
    ENGINE_ASSERT(reg.has<engine::Window>(window));
    reg.get<Controller::ControllableCamera>(e).window = window;
    return e;
}

void Controller::update(ecs::registry &reg)
{
    for(ecs::entity e_camera : reg.view<CAMERA_COMPONENTS>())
    {
        auto &controllable = reg.get<Controller::ControllableCamera>(e_camera);
        auto &camera = reg.get<engine::Camera>(e_camera);
        auto &listener = reg.get<engine::input::InputListener>(e_camera);
        auto &velocity = reg.get<engine::Velocity>(e_camera).values[m_uid];
        auto &orientation = static_cast<glm::quat &>(reg.get<engine::Orientation>(e_camera));
        auto &window = reg.get<engine::Window>(controllable.window);
        auto &viewMat = static_cast<glm::mat4 &>(reg.get<engine::ModelMatrix>(e_camera));

        glm::mat3 invViewMat = glm::inverse(glm::mat3(viewMat));
        glm::vec3 right   = invViewMat * glm::vec3{1, 0, 0};
        glm::vec3 up      = invViewMat * glm::vec3{0, 1, 0};
        glm::vec3 forward = invViewMat * glm::vec3{0, 0,-1};

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
                controllable.fov -= event.scroll * 4;
                controllable.fov = glm::clamp<float>(controllable.fov, 0.2, 45);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        camera.size = window.size;

        camera.projMat = glm::perspective<float>(glm::radians(controllable.fov), (float) camera.size.x / (float) camera.size.y, controllable.znear, controllable.zfar);
        viewMat = glm::mat4_cast(glm::normalize(orientation)) * glm::translate(glm::mat4(1.0f), -reg.get<engine::Position>(e_camera));
    }
}
