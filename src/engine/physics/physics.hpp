#pragma once
#include "engine/core/data.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/uid.hpp"
#include "glm/glm.hpp"
#include <unordered_map>

// TODO: refactor physics

namespace engine
{
    struct Velocity 
    {
        glm::vec3 value;
        std::unordered_map<UID, glm::vec3> values;
    };
    struct Acceleration 
    {
        glm::vec3 value;
        std::unordered_map<UID, glm::vec3> values;
    };

    class IPhysicsEngine
    {
    public:
        IPhysicsEngine() = default;
        virtual ~IPhysicsEngine() = default;
        virtual void update(ecs::registry &registry, float deltatime) = 0;
    };

    struct EnginePhysics : public IPhysicsEngine
    {
    private:
        void movement(ecs::registry &reg, float deltatime);
        engine::UID m_uid;
    public:
        void update(ecs::registry &registry, float deltatime) override;
    };
} // namespace engine::physics

