#include "Loaders.hpp"
#include "engine/data.hpp"

class IModelLoader
{
    IModelLoader() = default;
    virtual ~IModelLoader() = default;
    virtual engine::Model load(ecs::registry &reg);
};

ecs::entity engine::loader::detail::ModelLoader::load(ecs::registry &reg, nlohmann::json const &json)
{
    
}