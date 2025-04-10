#include "Controller.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"

void game::CameraController::update(double deltatime)
{
    for(auto entity : m_entities) {
        if(!ecs::entityHasComponent<Camera>(entity) || !ecs::entityHasComponent<Position>(entity)) continue;
        glm::vec3 &position = ecs::get<Position>(entity).position;
        glm::quat &cameraOrientation = ecs::get<RotationQuaternion>(entity).quat;
          if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            position += glm::vec3{0, 0, -1} * cameraOrientation * (float) deltatime;
        } if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            position -= glm::vec3{0, 0, -1} * cameraOrientation * (float) deltatime;
        } if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            position += glm::vec3{-1, 0, 0} * cameraOrientation * (float) deltatime;
        } if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            position -= glm::vec3{-1, 0, 0} * cameraOrientation * (float) deltatime;
        }


        if(!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
            firstTimeMovingMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            
            continue;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        glm::vec<2, double> cursorPos;
        glfwGetCursorPos(window, &cursorPos.x, &cursorPos.y);
        if(firstTimeMovingMouse) {
            firstTimeMovingMouse = false;
            m_prevCursorPos = cursorPos;
        }
        
        glm::vec2 offset = cursorPos - m_prevCursorPos;
        m_prevCursorPos = cursorPos;
        
        float sensitivity = 1;
        offset *= sensitivity;
        offset *= deltatime;

        cameraOrientation = glm::angleAxis(offset.y, glm::vec3{1, 0, 0}) * cameraOrientation * glm::angleAxis(offset.x, glm::vec3{0, 1, 0});

        if(cameraOrientation.x >= 1) {
            cameraOrientation.x = 0.999;
        } else if(cameraOrientation.x <= -1) {
            cameraOrientation.x = -0.999;
        }
    }
}