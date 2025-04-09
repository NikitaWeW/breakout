#include "Physics.hpp"

void game::MovementSystem::update(double deltatime)
{
    for(auto &entity : m_entities) {
        if(!(ecs::entityHasComponent<Position>(entity) && ecs::entityHasComponent<Velocity>(entity))) continue;
        ecs::get<Position>(entity).position += ecs::get<Velocity>(entity).velocity * (float) deltatime;
    }
}