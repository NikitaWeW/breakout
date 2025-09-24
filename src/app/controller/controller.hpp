#pragma once
#include "engine/config.hpp"
#include "engine/data.hpp"
#include "ecs.hpp"
#include "GLFW/glfw3.h"

namespace controller
{
    struct ControllableCamera
    {
        ecs::entity window;
        // contains engine::Window component
        float fov = 45;
        float speed = 1;
        float sensitivity = 1;
        float znear = 0.01;
        float zfar = 1000;
        bool firstTimeMovingMouse = true;
        bool locked = true;
    };

    ecs::entity createCamera(ecs::registry &reg, ecs::entity window);

    void update(ecs::registry &reg);
} // namespace engine::controller
