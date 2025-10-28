#pragma once
#include "engine/core/data.hpp"
#include "engine/core/ecs.hpp"
#include "glm/glm.hpp"

// TODO: refactor

namespace engine::physics
{
    struct MoveIntent : glm::vec3 {};

    void setup(ecs::registry &reg);
    void update(ecs::registry &reg, float deltatime);
} // namespace engine::physics

