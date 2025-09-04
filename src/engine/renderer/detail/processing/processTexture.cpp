#include "process.hpp"
#include "engine/data.hpp"
#include "engine/renderer/renderer.hpp"
#include "../ogl/Texture.hpp"
#include "engine/config.hpp"

void engine::renderer::detail::processTextures(ecs::registry &reg)
{
    ENGINE_PROFILE();
    auto textures = reg.view<engine::Texture>(ecs::exclude_t<Processed>{});

    for(auto const &e : textures)
    {
        auto const &texture = reg.get<engine::Texture>(e);
        auto const &textureEntity = e;

        ENGINE_ASSERT(texture.data.getData(), "Uninitialized texture!");
        ENGINE_ASSERT(texture.data.getWidth() * texture.data.getHeight(), "Empty texture!");

        reg.emplace<ogl::Texture>(textureEntity, texture.data, texture.type);
        reg.emplace<Processed>(e, textureEntity);
    }
}