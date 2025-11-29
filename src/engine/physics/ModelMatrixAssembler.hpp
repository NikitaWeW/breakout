#pragma once
#include "engine/core/ecs.hpp"

namespace engine
{
    struct ModelMatrixAssemblerExclude {};
    // Some systems like renderer expect entities with transform 
    // to have a matrix rather than position, orientation, etc.
    // TODO: a better transform system.
    class ModelMatrixAssembler
    {
    public:
        void update(ecs::registry &registry);
    };
} // namespace engine
