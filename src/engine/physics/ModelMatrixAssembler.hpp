#pragma once
#include "engine/core/ecs.hpp"

namespace engine
{
    struct ModelMatrixAssemblerExclude {};
    class ModelMatrixAssembler
    {
    public:
        void update(ecs::registry &registry);
    };
} // namespace engine
