#include "engine/Physics/Physics.hpp"

void engine::EnginePhysics::update(Registry &registry, float deltatime)
{
    movement(registry, deltatime);
}
