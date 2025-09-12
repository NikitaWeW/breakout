#pragma once
#include "engine/data.hpp"
#include "ogl.hpp"

namespace engine::renderer::detail
{
    struct MaterialTextures
    {
        ogl::Texture ambient;
        ogl::Texture diffuse;
        ogl::Texture specular;
        ogl::Texture bump;
        ogl::Texture displacement;
        ogl::Texture alpha;
        ogl::Texture reflection;
    };
    struct VertexBuffers
    {
        ogl::VBO positions;
        ogl::VBO texCoords;
        ogl::VBO normals;
        ogl::VBO tangents;
        ogl::VBO boneIDs;
        ogl::VBO weights;
    };
    struct Mesh
    {
        engine::Material material;
        detail::MaterialTextures textures;
        VertexBuffers buffers;
        ogl::VAO vao;
        ogl::IBO ibo;
        GLenum mode = GL_TRIANGLES;
        bool animated = false;

        unsigned count = 0;
    };
} // namespace engine::renderer::detail

