#pragma once
#include "engine/data.hpp"
#include "ogl/VertexBuffer.hpp"
#include "ogl/IndexBuffer.hpp"

namespace engine::renderer::detail
{
    struct Mesh
    {
        Material material;
        MaterialTextures textures;
        ogl::VertexBuffer vbo;
        ogl::VertexArray vao;
        ogl::IndexBuffer ibo;

        unsigned count = 0;
    };
} // namespace engine::renderer::detail

