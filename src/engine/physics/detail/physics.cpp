#include "physics.hpp"

static glm::vec3 getFinalVelocity(ecs::registry const &reg, ecs::entity e)
{
    glm::vec3 result = reg.get<engine::Velocity>(e);
    if(reg.has<engine::physics::MoveIntent>(e))
        result += reg.get<engine::physics::MoveIntent>(e);
    return result;
}

void detail::movement(ecs::registry &reg, float deltatime)
{
    for(ecs::entity e : reg.view<engine::Position, engine::Velocity>())
    {
        reg.get<engine::Position>(e) += getFinalVelocity(reg, e) * deltatime;
    }
}