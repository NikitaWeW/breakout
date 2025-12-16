#pragma once
#include "engine/DSA/Data.hpp"
#include "engine/DSA/ECS.hpp"
#include "engine/Header/UID.hpp"
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
        virtual void update(Registry &registry, float deltatime) = 0;
    };

    struct EnginePhysics : public IPhysicsEngine
    {
    private:
        void movement(Registry &reg, float deltatime);
        engine::UID m_uid;
    public:
        void update(Registry &registry, float deltatime) override;
    };
} // namespace engine::physics

