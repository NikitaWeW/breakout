#include "Controller.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"

void game::CameraController::update(double deltatime)
{
    for(auto entity : m_entities) {
        if(!ecs::entityHasComponent<Camera>(entity) || !ecs::entityHasComponent<ControllableCamera>(entity)) continue;
        ControllableCamera &controllable = ecs::get<ControllableCamera>(entity);
        Camera &camera = ecs::get<Camera>(entity);
        glfwGetWindowSize(controllable.window, &camera.width, &camera.height);
        if(!ecs::entityHasComponent<Position>(entity)) continue;

        glfwGetWindowSize(controllable.window, &camera.width, &camera.height);
        float const &speed = controllable.speedUnitsPerSecond;
        glm::mat4 const &invViewMat = glm::inverse(camera.viewMat);
        glm::vec3 &position = ecs::get<Position>(entity).position;
        
        glm::vec3 forward = glm::normalize(glm::vec3{invViewMat * glm::vec4{0, 0, -1, 0}});
        glm::vec3 right   = glm::normalize(glm::vec3{invViewMat * glm::vec4{1, 0, 0, 0}});
        glm::vec3 up      = glm::normalize(glm::vec3{invViewMat * glm::vec4{0, 1, 0, 0}});

          if(glfwGetKey(controllable.window, GLFW_KEY_W) == GLFW_PRESS) { // TODO: update velocity, not position
            position += speed * (float) deltatime * forward;
        } if(glfwGetKey(controllable.window, GLFW_KEY_S) == GLFW_PRESS) {
            position -= speed * (float) deltatime * forward;
        } if(glfwGetKey(controllable.window, GLFW_KEY_A) == GLFW_PRESS) {
            position -= speed * (float) deltatime * right;
        } if(glfwGetKey(controllable.window, GLFW_KEY_D) == GLFW_PRESS) {
            position += speed * (float) deltatime * right;
        } if(glfwGetKey(controllable.window, GLFW_KEY_E) == GLFW_PRESS) {
            position += speed * (float) deltatime * up;
        } if(glfwGetKey(controllable.window, GLFW_KEY_Q) == GLFW_PRESS) {
            position -= speed * (float) deltatime * up;
        } 

        if(!controllable.locked) {
            controllable.firstTimeMovingMouse = true;
            glfwSetInputMode(controllable.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            
            continue;
        } else {
            glfwSetInputMode(controllable.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        glm::vec<2, double> cursorPos;
        glfwGetCursorPos(controllable.window, &cursorPos.x, &cursorPos.y);
        if(controllable.firstTimeMovingMouse) {
            controllable.firstTimeMovingMouse = false;
            controllable.prevCursorPos = cursorPos;
        }
        
        glm::vec2 offset = cursorPos - controllable.prevCursorPos;
        controllable.prevCursorPos = cursorPos;
        
        offset *= controllable.sensitivity;
        offset *= deltatime;

        if(ecs::entityHasComponent<Rotation>(entity)) {
            glm::vec3 &orientation = ecs::get<Rotation>(entity).rotation;

            if(glfwGetKey(controllable.window, GLFW_KEY_Z) == GLFW_PRESS) {
                orientation.z -= controllable.sensitivity * (float) deltatime;
            } if(glfwGetKey(controllable.window, GLFW_KEY_C) == GLFW_PRESS) {
                orientation.z += controllable.sensitivity * (float) deltatime;
            } if(glfwGetKey(controllable.window, GLFW_KEY_X) == GLFW_PRESS) {
                orientation.z = 0;
            }

            orientation.x -= offset.y;
            orientation.y += offset.x;
    
            if(orientation.x >= 90) {
                orientation.x = 89.999;
            } else if(orientation.x <= -90) {
                orientation.x = -89.999;
            }
        }

        if(ecs::entityHasComponent<RotationQuaternion>(entity)) {
            glm::quat &orientation = ecs::get<RotationQuaternion>(entity).quat;
            if(glfwGetKey(controllable.window, GLFW_KEY_Z) == GLFW_PRESS) {
                orientation *= glm::angleAxis(glm::radians(controllable.sensitivity * (float) deltatime), glm::vec3{0, 0, 1});
            } if(glfwGetKey(controllable.window, GLFW_KEY_C) == GLFW_PRESS) {
                orientation *= glm::angleAxis(glm::radians(controllable.sensitivity * (float) deltatime), glm::vec3{0, 0, -1});
            } if(glfwGetKey(controllable.window, GLFW_KEY_X) == GLFW_PRESS) {
                orientation = glm::quat{1, 0, 0, 0};
            }

            orientation = glm::normalize(glm::angleAxis(glm::radians(offset.y), glm::vec3{1, 0, 0}) * orientation * glm::angleAxis(glm::radians(offset.x), glm::vec3{0, 1, 0}));
        }
    }
}