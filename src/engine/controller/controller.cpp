#include "controller.hpp"
#include "engine/input/input.hpp"
#include "engine/physics/physics.hpp"
#include "engine/data.hpp"

ecs::entity engine::controller::createCamera(ecs::registry &reg)
{
    return reg.create<engine::Camera, engine::controller::ControllableCamera, engine::physics::MoveIntent, engine::Velocity, engine::Position, engine::Orientation, engine::input::InputListener>();
}

void engine::controller::update(ecs::registry &reg)
{
    for(ecs::entity e_camera : reg.view<engine::Camera, engine::controller::ControllableCamera, engine::physics::MoveIntent, engine::Velocity, engine::Position, engine::Orientation, engine::input::InputListener>())
    {
        auto &controllable = reg.get<engine::controller::ControllableCamera>(e_camera);
        auto &camera = reg.get<engine::Camera>(e_camera);
        auto &listener = reg.get<engine::input::InputListener>(e_camera);
        auto &velocity = reg.get<engine::physics::MoveIntent>(e_camera);
        auto &orientation = reg.get<engine::Orientation>(e_camera);

        glm::mat4 invViewMat = glm::inverse(camera.viewMat);
        glm::vec3 right   = invViewMat * glm::vec4{1, 0, 0, 0};
        glm::vec3 up      = invViewMat * glm::vec4{0, 1, 0, 0};
        glm::vec3 forward = invViewMat * glm::vec4{0, 0,-1, 0};

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        for(; !listener.keyEvents.empty(); listener.keyEvents.pop())
        {
            auto const &event = listener.keyEvents.front();

            glm::vec3 movementDir{0};
            switch(event.key)
            {
                case GLFW_KEY_W: movementDir =  forward; break;
                case GLFW_KEY_S: movementDir = -forward; break;
                case GLFW_KEY_D: movementDir =  right;   break;
                case GLFW_KEY_A: movementDir = -right;   break;
                case GLFW_KEY_E: movementDir =  up;      break;
                case GLFW_KEY_Q: movementDir = -up;      break;
                default: break;
            }
            movementDir *= controllable.speed;
            
            if(event.action == GLFW_RELEASE)
            {
                velocity -= movementDir;
            }
            else if(event.action == GLFW_PRESS)
            {
                velocity += movementDir;
            }
        }

        for(; !listener.cursorPosEvents.empty(); listener.cursorPosEvents.pop())
        {
            auto const &event = listener.cursorPosEvents.front();
            glm::vec2 offset = event.cursorDeltaPos * controllable.sensitivity;

            orientation = engine::Orientation{glm::normalize(
                glm::angleAxis(glm::radians(offset.y), glm::vec3{1, 0, 0}) * 
                orientation * 
                glm::angleAxis(glm::radians(offset.x), glm::vec3{0, 1, 0})
            )};
        }

        for(; !listener.scrollEvents.empty(); listener.scrollEvents.pop())
        {
            auto const &event = listener.scrollEvents.front();

            controllable.fov += event.scroll;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        auto windows = reg.view<engine::Window>();
        ENGINE_ASSERT(!windows.empty(), "no windows found");
        auto &window = reg.get<engine::Window>(windows.at(0));
        camera.size = window.size;

        camera.projMat = glm::perspective<float>(glm::radians(controllable.fov), (float) camera.size.x / (float) camera.size.y, controllable.znear, controllable.zfar);
        camera.viewMat = glm::translate(glm::mat4_cast(glm::normalize(orientation)), -reg.get<engine::Position>(e_camera));
    }
}
