#include "engine/Physics/physics.hpp"

void engine::EnginePhysics::update(ecs::registry &registry, float deltatime)
{
    movement(registry, deltatime);
}
