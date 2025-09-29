#include <fstream>
#include <unordered_map>
#include "Loaders.hpp"

static std::string_view getDatatypeString(engine::DataType type)
{
    switch (type)
    {
    case engine::DataType::MODEL:     return "MODEL";
    case engine::DataType::TEXTURE2D: return "TEXTURE2D";
    case engine::DataType::MESH:      return "MESH";
    case engine::DataType::MATERIAL:  return "MATERIAL";
    case engine::DataType::DATA:      return "DATA";
    case engine::DataType::AUDIO:     return "AUDIO";
    default:                          return "UNKNOWN";
    }
}

ecs::entity engine::Loader::load(DataType type, std::string_view path) 
{
    ENGINE_CORE_TRACE("loading \"{}\"", path);

    if(m_loaderMap.find(type) == m_loaderMap.end())
    {
        ENGINE_CORE_ERROR("Unsupported type: \"{}\"!", getDatatypeString(type));
        ENGINE_CORE_ERROR("Supported types: ");
        for(auto const &[supported_type, loader] : m_loaderMap)
        {
            ENGINE_CORE_ERROR(getDatatypeString(supported_type));
        }
        if(m_loaderMap.empty())
        {
            ENGINE_CORE_ERROR("none D:");
        }
        ENGINE_ASSERT(false);
        return 0;
    }

    auto result = m_loaderMap.at(type)->load(*m_registry, path);
    if(result == 0)
        ENGINE_CORE_ERROR("Failed to load \"{}\"", path);
    return result;
}

engine::Loader::Loader(ecs::registry &registry)
{
    m_registry = &registry;
    m_loaders.emplace_back(std::move(std::make_unique<detail::ModelLoader>()));
    m_loaders.emplace_back(std::move(std::make_unique<detail::TextureLoader>()));

    m_loaderMap = {
        { DataType::MODEL,      m_loaders[0].get() },
        { DataType::TEXTURE2D,  m_loaders[1].get() } 
    };
}