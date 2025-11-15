#pragma once
#include "engine/core/ecs.hpp"

// Some systems like renderer expect entities with transform 
// to have a matrix rather than position, orientation, etc.
namespace engine
{
    struct ModelMatrixAssemblerExclude {};
    class ModelMatrixAssembler
    {
    public:
        void update(ecs::registry &registry);
    };
} // namespace engine
