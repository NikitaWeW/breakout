#pragma once
#include "engine/DSA/ECS.hpp"
#include "engine/Header/Handle.hpp"
#include "engine/Resource/Resources.hpp"
#include <string>

namespace engine
{

// FIXME: not the prettiest solution for option handling.

struct TextureLoaderOptions
{
    bool flip = true;
};
struct ModelLoaderOptions
{
    bool flipWindingOrder = false;
    bool flipUVs = false;
    TextureLoaderOptions textureOptions;
};
struct CubemapLoaderOptions : public TextureLoaderOptions {};


// TODO: Replace ecs::entity with engine::Entity
class ModelLoader : private Handle<struct ModelLoaderImpl>
{
public:
    /// @brief Construct an invalid loader.
    ModelLoader() = default;
    explicit ModelLoader(Registry &reg);
    ecs::entity loadFromFile(std::string_view path, ModelLoaderOptions options = {});
    ecs::entity loadFromMemory(void const *data, size_t size, ModelLoaderOptions options = {});
    Material getDefaultMaterial() const;
};

class TextureLoader : private Handle<struct TextureLoaderImpl>
{
public:
    TextureLoader() = default;
    explicit TextureLoader(Registry &reg);
    ecs::entity loadFromFile(std::string_view path, TextureLoaderOptions options = {});
    ecs::entity loadFromMemory(void const *data, size_t size, TextureLoaderOptions options = {});
};

class CubemapLoader : private Handle<struct CubemapLoaderImpl>
{
public:
    CubemapLoader() = default;
    explicit CubemapLoader(Registry &reg);
    ecs::entity loadFromFile(std::string_view path, CubemapLoaderOptions options = {});
    ecs::entity loadFromMemory(void const *data, size_t size, CubemapLoaderOptions options = {});
};

} // namespace engine
