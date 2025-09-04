#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "json.hpp"

namespace engine::loader::detail
{
    class ILoader
    {
    public:
        ILoader() = default;
        virtual ~ILoader() = default;
        virtual ecs::entity load(ecs::registry &reg, nlohmann::json const &json) = 0;
    };
} // namespace engine::loader::detail
