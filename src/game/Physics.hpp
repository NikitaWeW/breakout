#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "utils/ECS.hpp"

namespace game
{
    struct Position
    {
        glm::vec3 position;
    };
    struct Rotation
    {
        glm::vec3 rotation;
    };
    struct Scale
    {
        glm::vec3 scale;
    };
    struct Velocity
    {
        glm::vec3 velocity;
    };
    struct RotationQuaternion
    {
        glm::quat quat;
    };

    class MovementSystem : public ecs::System
    {
    public:
        void update(double deltatime);
    };  
} // namespace game
