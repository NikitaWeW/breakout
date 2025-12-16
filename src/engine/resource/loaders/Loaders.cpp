#pragma once
#include "engine/Resource/Resources.hpp"
#include "Loaders.hpp"

namespace engine
{

void addLoadersToResourceManager()
{
    auto &mgr = ResourceManager::instance();
    mgr.registerLoader(ResourceType::MODEL,   std::make_unique<ModelLoader>());
    mgr.registerLoader(ResourceType::BITMAP,  std::make_unique<TextureLoader>());
    mgr.registerLoader(ResourceType::CUBEMAP, std::make_unique<CubemapLoader>());
}

}