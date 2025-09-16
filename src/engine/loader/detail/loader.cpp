#include "engine/loader/loader.hpp"
#include "engine/data.hpp"
#include "engine/config.hpp"
#include "../loader.hpp"

#include <fstream>
#include <unordered_map>

#include "Loaders.hpp"

static std::string_view getExtension(std::string_view path)
{
    if(path.find_last_of(".") != std::string::npos)
        return path.substr(path.find_last_of(".") + 1);
    return "";
}

ecs::entity engine::loader::load(ecs::registry &reg, std::string_view path) 
{
    ENGINE_ASSERT(reg.view<detail::LoaderData>().size() == 1, "forgot to call engine::loader::setup() / called more than once?");
    auto &data = reg.get<detail::LoaderData>(reg.view<detail::LoaderData>().at(0));
    std::string_view extension = getExtension(path);

    if(data.loaderMap.find(extension) == data.loaderMap.end())
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
        ENGINE_OUT << ".\n";
        ENGINE_ASSERT(false, "Unrecognised extension");
        return 0;
    }

    return data.loaderMap.at(extension)->load(reg, path);
}

void engine::loader::setup(ecs::registry &reg)
{
    auto e = reg.create<detail::LoaderData>();
    auto &data = reg.get<detail::LoaderData>(e);

    data.loaders.emplace_back(std::move(std::make_unique<detail::ObjModelLoader>()));
    data.loaders.emplace_back(std::move(std::make_unique<detail::ObjModelLoader>()));
    data.loaders.emplace_back(std::move(std::make_unique<detail::TextureLoader>()));

    data.loaderMap = {
        { "obj",  data.loaders[0].get() },

        { "gltf", data.loaders[1].get() },
        { "glb",  data.loaders[1].get() },


        { "png",  data.loaders[2].get() },
        { "jpg",  data.loaders[2].get() },
        { "jpeg", data.loaders[2].get() },
        { "bmp",  data.loaders[2].get() },
        { "tga",  data.loaders[2].get() },
        { "psd",  data.loaders[2].get() },
        { "gif",  data.loaders[2].get() },
        { "hdr",  data.loaders[2].get() },
        { "pic",  data.loaders[2].get() },
        { "pnm",  data.loaders[2].get() } 
    };

    data.defaultMaterial = {
        .ambient       = {0.1f, 0.1f, 0.1f},
        .diffuse       = {0.8f, 0.8f, 0.8f},
        .specular      = {0.5f, 0.5f, 0.5f},
        .transmittance = {0.0f, 0.0f, 0.0f},
        .emission      = {0.0f, 0.0f, 0.0f},

        .shininess = 32.0f,
        .ior       = 1.5f
    }; 

    ecs::entity white = reg.create(Texture{
        .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
            1, 1, 1
        }.data()},
        .type = "",
        .grayscale = true,
        .path = ""
    });
    ecs::entity blue = reg.create(Texture{
        .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
            0, 0, 1
        }.data()},
        .type = "",
        .grayscale = true,
        .path = ""
    });
    ecs::entity black = reg.create(Texture{
        .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
            0, 0, 0
        }.data()},
        .type = "",
        .grayscale = true,
        .path = ""
    });
    ecs::entity tile = reg.create(Texture{
        .data = engine::Bitmap<float>{2, 2, 3, std::array<float, 4*3>{
            1, 1, 1, 0.5, 0.5, 0.5,
            0.5, 0.5, 0.5, 1, 1, 1 
        }.data()},
        .type = "",
        .grayscale = true,
        .path = ""
    });

    data.defaultTextures = {
        .ambient = white,
        .diffuse = tile,
        .specular = white,
        .bump = blue,
        .displacement = black,
        .alpha = white,
        .reflection = black
    };
}
