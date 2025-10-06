#pragma once
#include "engine/data.hpp"
#include "engine/config.hpp"
#include "ecs.hpp"
#include "glm/glm.hpp"

// TODO: refactor

namespace engine::physics
{
    struct MoveIntent : glm::vec3 {};

    void setup(ecs::registry &reg);
    void update(ecs::registry &reg, float deltatime);
} // namespace engine::physics

