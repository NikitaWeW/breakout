#include "engine/loader/loader.hpp"
#include "engine/data.hpp"
#include "engine/config.hpp"
#include "../loader.hpp"

#include <fstream>
#include <unordered_map>

#include "Loaders.hpp"

struct LoaderData {
    std::vector<std::unique_ptr<engine::loader::detail::ILoader>> loaders;
    std::unordered_map<std::string_view, engine::loader::detail::ILoader *> loaderMap;
};

static std::string_view getExtension(std::string_view path)
{
    if(path.find_last_of(".") != std::string::npos)
        return path.substr(path.find_last_of(".") + 1);
    return "";
}
static LoaderData &getLoaderData(ecs::registry &reg)
{
    auto loaderMaps = reg.view<LoaderData>();
    ENGINE_ASSERT(loaderMaps.size() == 1, "Incorrect number of loader maps (has to be one)!");

    return reg.get<LoaderData>(loaderMaps[0]);
}

ecs::entity engine::loader::load(ecs::registry &reg, std::string_view path) 
{
    auto &data = getLoaderData(reg);
    std::string_view extension = path.substr(0, path.find_last_of('.'));

    if(data.loaderMap.find(extension) != data.loaderMap.end())
    {
        ENGINE_OUT << "Unrecognised extension: \"" << extension << "\"!\n";
        ENGINE_OUT << "Supported extensions: ";
        bool first = true;
        for(auto const &[extension, loader] : data.loaderMap)
        {
            if(!first)
            {
                ENGINE_OUT << ", ";
            }
            ENGINE_OUT << extension;
            first = false;
        }
        if(first)
        {
            ENGINE_OUT << "none D:";
        }
        ENGINE_OUT << '\n';
        ENGINE_ASSERT(false, "Unrecognised extension");
        return 0;
    }

    return data.loaderMap.at(extension)->load(reg, path);
}

void engine::loader::setup(ecs::registry &reg)
{
    auto e = reg.create<LoaderData>();
    auto &data = reg.get<LoaderData>(e);
    data.loaders.emplace_back(std::move(std::make_unique<detail::ObjModelLoader>()));
    data.loaders.emplace_back(std::move(std::make_unique<detail::GLTFModelLoader>()));
    data.loaderMap.emplace("obj",  data.loaders[0].get());
    data.loaderMap.emplace("gltf", data.loaders[1].get());
    data.loaderMap.emplace("glb",  data.loaders[1].get());
}
