#include "engine/physics/detail/physics.hpp"

void engine::physics::update(ecs::registry &reg, float deltatime)
{
    detail::movement(reg, deltatime);
}