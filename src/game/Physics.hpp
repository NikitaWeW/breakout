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
    struct OrientationEuler
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
    struct OrientationQuaternion
    {
        glm::quat quat;
    };
    struct Direction
    {
        glm::vec3 dir;
    };

    class MovementSystem : public ecs::ISystem
    {
    public:
        void update(std::set<ecs::Entity_t> const &entities, double deltatime) override;
    };  
} // namespace game
