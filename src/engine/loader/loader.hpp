#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"

namespace engine::loader
{
    void setup(ecs::registry &reg);

    /**
     * \brief Load an object.
     * \return The entity containing the object data.
     */
    ecs::entity load(ecs::registry &reg, std::string_view path);
} // namespace engine::loader
