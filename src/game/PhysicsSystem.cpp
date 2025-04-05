#include "PhysicsSystem.hpp"
#include "PhysicsComponents.hpp"

void game::MovementSystem::update(double deltatime)
{
    for(auto &entity : m_entities) {
        if(!(ecs::entityHasComponent<PositionComponent>(entity) && ecs::entityHasComponent<VelocityComponent>(entity))) continue;
        ecs::get<PositionComponent>(entity).position += ecs::get<VelocityComponent>(entity).velocity * (float) deltatime;
    }
}