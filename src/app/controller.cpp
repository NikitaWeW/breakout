#include "controller.hpp"
#include "glm/gtx/io.hpp"

#define CAMERA_COMPONENTS engine::Camera, engine::Transform, Controller::ControllableCamera, engine::Velocity, engine::input::InputListener

ecs::entity Controller::createCamera(engine::Registry &reg, glm::vec3 pos, glm::vec3 target)
{
    auto e = reg.create<CAMERA_COMPONENTS>();
    reg.get<Controller::ControllableCamera>(e).window = reg.view<engine::Window>().at(0);
    auto up = abs(glm::dot(glm::normalize(target - pos), glm::vec3{0,1,0})) > 0.99 ? glm::vec3{1,0,0} : glm::vec3{0,1,0};
    reg.get<engine::Transform>(e) = {
        .position = pos,
        .orientation = glm::quat_cast(glm::lookAt(pos, target, up))
    };
    return e;
}

void Controller::update(engine::Registry &reg)
{
    for(ecs::entity e_camera : reg.view<CAMERA_COMPONENTS>())
    {
        auto &controllable = reg.get<Controller::ControllableCamera>(e_camera);
        auto &camera = reg.get<engine::Camera>(e_camera);
        auto &listener = reg.get<engine::input::InputListener>(e_camera);
        auto &velocity = reg.get<engine::Velocity>(e_camera).values[m_uid];
        auto &transform = reg.get<engine::Transform>(e_camera);
        auto &window = reg.get<engine::Window>(controllable.window);

        glm::mat3 invViewMat = glm::inverse(glm::mat3(camera.viewMat));
        glm::vec3 right   = invViewMat * glm::vec3{1, 0, 0};
        glm::vec3 up      = invViewMat * glm::vec3{0, 1, 0};
        glm::vec3 forward = invViewMat * glm::vec3{0, 0,-1};
        // ENGINE_TRACE(fmt::streamed(forward));

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
        velocity *= controllable.speed * (glfwGetKey(window.glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? controllable.boost : 1);

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
                    transform.orientation * 
                    glm::angleAxis(glm::radians(offset.x), glm::vec3{0, 1, 0})
                );
                if(glm::abs(glm::vec3(glm::inverse(glm::mat4_cast(newOrientation)) * glm::vec4{0,0,-1,0}).y) < 0.99)
                    transform.orientation = {newOrientation};
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
        camera.viewMat = glm::mat4_cast(glm::normalize(transform.orientation)) * glm::translate(glm::mat4(1.0f), -transform.position);
    }
}
