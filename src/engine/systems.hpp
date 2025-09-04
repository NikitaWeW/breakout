#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include <functional>
#include <vector>

namespace engine
{
    using SystemFunc = std::function<void(ecs::registry &, float)>;

    /**
     * \brief A class to manage ecs systems.
     */
    class SystemManager
    {
    private:
        ecs::registry m_registry;
        std::vector<SystemFunc> m_systems;
    public:
        /** \brief Move in a new registry. */
        inline void setRegistry(ecs::registry &&reg) { m_registry = std::move(reg); }

        inline ecs::registry &getRegistry() { return m_registry; }
        inline ecs::registry const &getRegistry() const { return m_registry; }

        template<typename... Args>
        inline void emplaceSystem(Args&&... args)
        {
            m_systems.emplace_back(std::forward<Args>(args)...);
        }

        inline void update(float dt)
        {
            for(auto const &system : m_systems)
            {
                system(m_registry, dt);
            }
        }
    }; 
} // namespace engine

