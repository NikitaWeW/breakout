#include "engine/loader/loader.hpp"
#include "ILoader.hpp"
#include "engine/data.hpp"
#include "engine/config.hpp"
#include "loader.hpp"

#include <fstream>
#include <unordered_map>

#include "Loaders.hpp"

struct LoaderMap : std::unordered_map<std::string, std::unique_ptr<engine::loader::detail::ILoader>> {};

static LoaderMap &getLoaderMap(ecs::registry &reg)
{
    auto loaderMaps = reg.view<LoaderMap>();
    ENGINE_ASSERT(loaderMaps.size() == 1, "Incorrect number of loader maps (has to be one)!");

    return reg.get<LoaderMap>(loaderMaps[0]);
}

ecs::entity engine::loader::load(ecs::registry &reg, std::string_view path) 
{
    using namespace nlohmann;

    auto &loaders = getLoaderMap(reg);

    std::ifstream filestream{std::string{path}};
    if(!filestream) {
        std::cout << "[loader] failed to open file \"" << path << "\"!\n";
        assert(false);
    }
    
    json data = json::parse(filestream);

    std::string type = data.contains("type") ? data["type"].get<std::string>() : "undefined";

    if(loaders.find(type) != loaders.end())
    {
        ENGINE_OUT << "[loader] Unrecognised type: \"" << type << "\"!\n";
        ENGINE_ASSERT(false, "");
        return 0;
    }

    return loaders.at(type)->load(reg, data);
}

void engine::loader::setup(ecs::registry &reg)
{
    auto e = reg.create<LoaderMap>();
    auto &loaderMap = reg.get<LoaderMap>(e);
    loaderMap.emplace("model", std::make_unique<detail::ModelLoader>());
}
