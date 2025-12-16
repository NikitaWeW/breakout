#include "engine/Physics/Physics.hpp"
#include "engine/Logging/logging.hpp"

void engine::EnginePhysics::movement(Registry &reg, float deltatime)
{
    for(auto e : reg.view<engine::Transform, Velocity>())
    {
        auto &velocity = reg.get<Velocity>(e);
        if(reg.has<Acceleration>(e))
        {
            auto &acceleration = reg.get<Acceleration>(e);
            acceleration.value = glm::vec3{0};
            for(auto const &[uid, add] : acceleration.values)
                acceleration.value += add;
            
            velocity.values[m_uid] += acceleration.value * deltatime;
        }

        velocity.value = glm::vec3{0};
        for(auto const &[uid, add] : velocity.values)
            velocity.value += add;

        engine::Transform &transform = reg.get<engine::Transform>(e);
        transform.position += velocity.value * deltatime;
    }
}