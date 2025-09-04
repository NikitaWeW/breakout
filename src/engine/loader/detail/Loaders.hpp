#pragma once
#include "ILoader.hpp"

namespace engine::loader::detail
{
    class ModelLoader : public ILoader
    {
    public:
        ecs::entity load(ecs::registry &reg, nlohmann::json const &json) override;
    };
} // namespace engine::loader::detail
