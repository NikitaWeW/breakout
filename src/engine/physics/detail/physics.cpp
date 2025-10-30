#include "detail.hpp"
#include "engine/physics/physics.hpp"

void engine::EnginePhysics::update(ecs::registry &registry, float deltatime)
{
    physics::movement(registry, deltatime);
}
