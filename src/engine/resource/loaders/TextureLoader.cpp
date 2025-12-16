#include "Loaders.hpp"
#include "stb_image.h"
#include "engine/DSA/Equirect.hpp"

using namespace engine;

std::unique_ptr<IResource> TextureLoader::loadFromFile(std::string_view path, LoaderOptions_t o)
{
    auto options = castOptions<TextureLoaderOptions>(o);

    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(options.flip);
    float *buff = stbi_loadf(path.data(), &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture: \"{}\"!: {}", path, stbi_failure_reason());
        return nullptr;
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    Texture texture;
    texture.data = Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    stbi_image_free(buff);

    return std::make_unique<IResource>(std::move(texture));
}
std::unique_ptr<IResource> TextureLoader::loadFromMemory(void const *data, size_t size, LoaderOptions_t o)
{
    auto options = castOptions<TextureLoaderOptions>(o);
    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(options.flip);
    float *buff = stbi_loadf_from_memory(static_cast<unsigned char const *>(data), size, &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture from memory: {}", stbi_failure_reason());
        return nullptr;
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    Texture texture;
    texture.data = Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    stbi_image_free(buff);

    return std::make_unique<IResource>(std::move(texture));
}

std::unique_ptr<IResource> CubemapLoader::loadFromFile(std::string_view path, LoaderOptions_t o)
{
    auto texture = castResource<Texture>(TextureLoader::loadFromFile(path, o));
    if(!texture)
        return nullptr;

    Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture->data);

    return std::make_unique<IResource>(std::move(cubemap));
}
std::unique_ptr<IResource> CubemapLoader::loadFromMemory(void const *data, size_t size, LoaderOptions_t o)
{
    auto texture = castResource<Texture>(TextureLoader::loadFromMemory(data, size, o));
    if(!texture)
        return nullptr;

    Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture->data);

    return std::make_unique<IResource>(std::move(cubemap));
}
