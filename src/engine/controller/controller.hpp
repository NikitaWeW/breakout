#pragma once
#include "engine/config.hpp"
#include "engine/data.hpp"
#include "ecs.hpp"
#include "GLFW/glfw3.h"

namespace engine::controller
{
    struct ControllableCamera
    {
        // contains engine::Window component
        ecs::entity window;
        float fov = 45;
        float speed = 1;
        float sensitivity = 1;
        float znear = 0.01;
        float zfar = 1000;
        bool firstTimeMovingMouse = true;
    };

    ecs::entity createCamera(ecs::registry &reg);

    void update(ecs::registry &reg);
} // namespace engine::controller
