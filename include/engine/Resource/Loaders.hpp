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
    bool flip = true; /// Flip the image vertically, so the first pixel in the output array is the bottom left.
    bool cache = true; /// Cache the texture into uncompressed .bmp file for faster loading.
};
struct ModelLoaderOptions
{
    bool flipWindingOrder = false; /// Flip the winding model of the triangles.
    bool flipUVs = false; /// Flip the texture coordinates vertically.
    TextureLoaderOptions textureOptions; /// Options for texture loading.
};
struct CubemapLoaderOptions 
{
    TextureLoaderOptions textureOptions; /// Options for cubemap image loading.
};


// TODO: Replace ecs::entity with engine::Entity
class ModelLoader : private Handle<struct ModelLoaderImpl>
{
public:
    /// @brief Construct an invalid loader.
    ModelLoader() = default;

    /// @brief Construct a valid loader.
    explicit ModelLoader(Registry &reg);

    /// @brief Load a model from file.
    /// @param path The path to the file 
    /// @param options The options for loading the model.
    ecs::entity loadFromFile(std::string_view path, ModelLoaderOptions options = {});

    /// @brief Load a model from bytes.
    /// @param data The pointer to the model data.
    /// @param size The size of @p data.
    /// @param options The options for loading the model.
    ecs::entity loadFromMemory(void const *data, size_t size, ModelLoaderOptions options = {});

    /// @brief Get the default material.
    /// This material may be (partially) applied to meshes without some parameters or textures.
    /// Is guaranteed to have all the parameters and textures set.
    Material getDefaultMaterial() const;
};

class TextureLoader : private Handle<struct TextureLoaderImpl>
{
public:
    /// @brief Construct an invalid loader.
    TextureLoader() = default;

    /// @brief Construct a valid loader.
    explicit TextureLoader(Registry &reg);

    /// @brief Load a texture from file.
    /// @param path The path to the file 
    /// @param options The options for loading the texture.
    ecs::entity loadFromFile(std::string_view path, TextureLoaderOptions options = {});

    /// @brief Load a texture from bytes.
    /// @param data The pointer to the texture data.
    /// @param size The size of @p data.
    /// @param options The options for loading the texture.
    ecs::entity loadFromMemory(void const *data, size_t size, TextureLoaderOptions options = {});
};

class CubemapLoader : private Handle<struct CubemapLoaderImpl>
{
public:
    /// @brief Construct an invalid loader.
    CubemapLoader() = default;

    /// @brief Construct a valid loader.
    explicit CubemapLoader(Registry &reg);

    /// @brief Load an equirectangular cubemap from file.
    /// @param path The path to the file 
    /// @param options The options for loading the cubemap.
    ecs::entity loadFromFile(std::string_view path, CubemapLoaderOptions options = {});

    /// @brief Load an equirectangular cubemap from bytes.
    /// @param data The pointer to the image data.
    /// @param size The size of @p data.
    /// @param options The options for loading the cubemap.
    ecs::entity loadFromMemory(void const *data, size_t size, CubemapLoaderOptions options = {});
};

} // namespace engine
