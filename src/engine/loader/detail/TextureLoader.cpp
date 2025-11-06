#include "Loaders.hpp"
#include "stb_image.h"
#include "engine/core/equirect.hpp"

static std::optional<engine::Texture> loadTexture(std::string_view path, engine::LoadingFlags flags)
{
    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(static_cast<int>(flags & engine::LoadingFlags::FLIP_TEXTURES));
    float *buff = stbi_loadf(path.data(), &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture: \"{}\"!: {}", path, stbi_failure_reason());
        return {};
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    engine::Texture texture;
    texture.path = path;
    texture.data = engine::Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    stbi_image_free(buff);

    return texture;
}
static std::optional<engine::Texture> loadTexture(std::size_t size, void const *data, engine::LoadingFlags flags)
{
    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(static_cast<int>(flags & engine::LoadingFlags::FLIP_TEXTURES));
    float *buff = stbi_loadf_from_memory(static_cast<unsigned char const *>(data), size, &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture from memory: {}", stbi_failure_reason());
        return {};
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    engine::Texture texture;
    texture.path = "memory";
    texture.data = engine::Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    stbi_image_free(buff);

    return texture;
}

ecs::entity engine::loader::TextureLoader::load(ecs::registry &reg, std::string_view path, LoadingFlags flags)
{
    auto texture = loadTexture(path, flags);
    if(!texture.has_value())
        return 0;
    return reg.create(std::move(texture.value()));
}
ecs::entity engine::loader::TextureLoader::load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags)
{
    auto texture = loadTexture(size, data, flags);
    if(!texture.has_value())
        return 0;
    return reg.create(std::move(texture.value()));
}


ecs::entity engine::loader::CubemapLoader::load(ecs::registry &reg, std::string_view path, LoadingFlags flags)
{
    auto texture = loadTexture(path, flags);
    if(!texture.has_value())
        return 0;

    engine::Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture->data);
    cubemap.path = path;

    return reg.create(std::move(cubemap));
}
ecs::entity engine::loader::CubemapLoader::load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags)
{
    auto texture = loadTexture(size, data, flags);
    if(!texture.has_value())
        return 0;

    engine::Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture->data);
    cubemap.path = "memory";

    return reg.create(std::move(cubemap));
}
