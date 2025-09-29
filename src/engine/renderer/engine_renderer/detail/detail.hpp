#pragma once
#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "ogl.hpp"

namespace engine::detail
{
    struct Processed 
    {
        ecs::entity data;
    };

    struct ProcessedModel : public Processed {};
    struct ProcessedTexture : public Processed {};

    
    struct MaterialTextures
    {
        ogl::Texture ambient;
        ogl::Texture albedo;
        ogl::Texture specular;
        ogl::Texture normal;
        ogl::Texture displacement;
        ogl::Texture alpha;
        ogl::Texture reflection;
        ogl::Texture metallic;
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
        engine::Mesh::Skeleton skeleton;
        engine::Material::Properties material;
        detail::MaterialTextures textures;
        VertexBuffers buffers;
        ogl::VAO vao;
        ogl::IBO ibo;
        GLenum mode = GL_TRIANGLES;
        bool animated = false;

        unsigned count = 0;
    };
    struct Model
    {
        std::vector<detail::Mesh> meshes;
    };

    struct RendererData
    {
        ogl::Framebuffer oitFBO;
        ogl::Texture oitAccumTexture;
        ogl::Texture oitRevelageTexture;

        ogl::Framebuffer mainFBO;
        ogl::Texture mainFBOColor;
        ogl::Renderbuffer mainFBORBO;

        ogl::Program plainColorShader;

        glm::uvec2 prevCamSize{0};
    }; 
} // namespace engine::detail
