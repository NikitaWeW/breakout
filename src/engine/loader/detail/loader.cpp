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
    case engine::DataType::BUFFER:    return "BUFFER";
    case engine::DataType::AUDIO:     return "AUDIO";
    default:                          return "UNKNOWN";
    }
}

ecs::entity engine::Loader::load(DataType type, std::string_view path, LoadingFlags flags) 
{
    ENGINE_CORE_TRACE("Loading \"{}\"", path);

    if(!checkType(type)) 
        return 0;

    auto result = m_loaderMap.at(type)->load(*m_registry, path, flags);
    if(result == 0)
        ENGINE_CORE_ERROR("Failed to load \"{}\"", path);
    return result;
}

ecs::entity engine::Loader::load(DataType type, std::size_t size, void const *data, LoadingFlags flags)
{
    ENGINE_CORE_TRACE("Loading {} from memory", getDatatypeString(type));

    if(!checkType(type)) 
        return 0;

    auto result = m_loaderMap.at(type)->load(*m_registry, size, data, flags);
    if(result == 0)
        ENGINE_CORE_ERROR("Failed to load {} from memory", getDatatypeString(type));
    return result;
}
bool engine::Loader::checkType(DataType type)
{
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
        return false;
    }
    return true;
}
engine::Loader::Loader(ecs::registry &registry)
{
    m_registry = &registry;
    registerLoader(DataType::MODEL,     std::make_unique<loader::ModelLoader>());
    registerLoader(DataType::TEXTURE2D, std::make_unique<loader::TextureLoader>());
}
