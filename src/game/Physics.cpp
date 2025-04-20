#include "Physics.hpp"

void game::MovementSystem::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &entity : entities) {
        if(!(ecs::entityHasComponent<Position>(entity) && ecs::entityHasComponent<Velocity>(entity))) continue;
        ecs::get<Position>(entity).position += ecs::get<Velocity>(entity).velocity * (float) deltatime;
    }
}