#pragma once
#include "glm/glm.hpp"
#include "utils/ECS.hpp"

namespace game
{
    class MovementSystem : public ecs::System
    {
    public:
        void update(double deltatime);
    };  
} // namespace game
