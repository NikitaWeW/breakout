#pragma once
#include "engine/Engine.hpp"
#include "GLFW/glfw3.h"

class Controller
{
private:
    engine::UID m_uid;
public:
    struct ControllableCamera
    {
        ecs::entity window;
        // contains engine::Window component
        float fov = 45;
        float speed = 1;
        float boost = 10;
        float sensitivity = 1;
        float znear = 0.01;
        float zfar = 1000;
        bool firstTimeMovingMouse = true;
        bool locked = true;
    };

    static ecs::entity createCamera(Registry &reg, glm::vec3 pos = {0, 0, 0}, glm::vec3 target = {0, 0, -10});

    void update(Registry &reg);
};
