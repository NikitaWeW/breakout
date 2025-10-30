#include "detail.hpp"

void engine::physics::movement(ecs::registry &reg, float deltatime)
{
    for(auto e : reg.view<Position, Velocity>())
    {
        auto &velocity = reg.get<Velocity>(e);
        velocity.value = glm::vec3{0};
        for(auto const &[uid, add] : velocity.values)
            velocity.value += add;

        if(reg.has<Acceleration>(e))
        {
            auto &acceleration = reg.get<Acceleration>(e);
            acceleration.value = glm::vec3{0};
            for(auto const &[uid, add] : acceleration.values)
                acceleration.value += add;

            velocity.value += reg.get<Acceleration>(e).value * deltatime;
        }

        reg.get<Position>(e) += velocity.value * deltatime;
    }
}