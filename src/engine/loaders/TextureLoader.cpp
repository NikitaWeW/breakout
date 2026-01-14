#include "engine/Resource/Loaders.hpp"
#include "engine/Resource/Resources.hpp"
#include "engine/Logging/Logging.hpp"
#include "stb_image.h"
#include "engine/DSA/Equirect.hpp"

using namespace engine;

struct engine::TextureLoaderImpl
{
    Registry *mReg = nullptr;
};

engine::TextureLoader::TextureLoader(Registry &reg) : Handle(new TextureLoaderImpl)
{ 
    unwrap().mReg = &reg; 
}
ecs::entity TextureLoader::loadFromFile(std::string_view path, TextureLoaderOptions options)
{
    ENGINE_ASSERT_MSG(!this->empty(), "Invalid loader! (make sure to not use default constructor when making an actual loader)");
    for(auto e_texture : unwrap().mReg->view<Texture>())
        if(unwrap().mReg->get<Texture>(e_texture).path == path)
            return e_texture;

    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(options.flip);
    float *buff = stbi_loadf(path.data(), &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture: \"{}\"!: {}", path, stbi_failure_reason());
        return INVALID_ENTITY;
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    Texture texture;
    texture.data = Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    texture.path = path;
    stbi_image_free(buff);

    return unwrap().mReg->create(std::move(texture));
}
ecs::entity TextureLoader::loadFromMemory(void const *data, size_t size, TextureLoaderOptions options)
{
    ENGINE_ASSERT_MSG(!this->empty(), "Invalid loader! (make sure to not use default constructor when making an actual loader)");
    int width = 0, height = 0, numChannels = 0;
    stbi_set_flip_vertically_on_load(options.flip);
    float *buff = stbi_loadf_from_memory(static_cast<unsigned char const *>(data), size, &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_CORE_ERROR("failed to load texture from memory: {}", stbi_failure_reason());
        return INVALID_ENTITY;
    }
    ENGINE_ASSERT_MSG(width > 0 && height > 0, "failed to load a texture");
    Texture texture;
    texture.data = Bitmap{(unsigned) width, (unsigned) height, (unsigned) numChannels, buff};
    texture.path = "loadFromMemory";
    stbi_image_free(buff);

    return unwrap().mReg->create(std::move(texture));
}

struct engine::CubemapLoaderImpl
{
    TextureLoader mTextureLoader;
    Registry *mReg = nullptr;
};
CubemapLoader::CubemapLoader(Registry &reg) : Handle(new CubemapLoaderImpl{})
{ 
    unwrap().mReg = &reg; 
    unwrap().mTextureLoader = TextureLoader{*unwrap().mReg};
}
ecs::entity CubemapLoader::loadFromFile(std::string_view path, CubemapLoaderOptions options)
{
    ENGINE_ASSERT_MSG(!this->empty(), "Invalid loader! (make sure to not use default constructor when making an actual loader)");
    for(auto e_cubemap : unwrap().mReg->view<Cubemap>())
        if(unwrap().mReg->get<Cubemap>(e_cubemap).path == path)
            return e_cubemap;

    auto e_texture = unwrap().mTextureLoader.loadFromFile(path, options.textureOptions);
    if(!e_texture)
        return INVALID_ENTITY;

    auto const &texture = unwrap().mReg->get<Texture>(e_texture);

    Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture.data);
    cubemap.path = texture.path;

    return unwrap().mReg->create(std::move(cubemap));
}
ecs::entity CubemapLoader::loadFromMemory(void const *data, size_t size, CubemapLoaderOptions options)
{
    ENGINE_ASSERT_MSG(!this->empty(), "Invalid loader! (make sure to not use default constructor when making an actual loader)");
    auto e_texture = unwrap().mTextureLoader.loadFromMemory(data, size, options.textureOptions);
    if(!e_texture)
        return INVALID_ENTITY;

    auto const &texture = unwrap().mReg->get<Texture>(e_texture);

    Cubemap cubemap;
    cubemap.faces = eqr::toCubemap(texture.data);
    cubemap.path = texture.path;

    return unwrap().mReg->create(std::move(cubemap));
}
