#pragma once
#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "ogl.hpp"
#include "engine/animation/animation.hpp"

namespace engine::renderer
{
    struct Processed 
    {
        ecs::entity data;
    };

    struct ProcessedModel : public Processed {};
    struct ProcessedTexture : public Processed {};

    
    struct MaterialTextures
    {
        ogl::Texture albedo;
        ogl::Texture normal;
        ogl::Texture metallic;
        ogl::Texture roughness;
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
        engine::Material::Properties material;
        MaterialTextures textures;
        VertexBuffers buffers;
        ogl::VAO vao;
        ogl::IBO ibo;
        GLenum mode = GL_TRIANGLES;

        unsigned count = 0;
    };
    struct Model
    {
        engine::Skeleton skeleton;
        std::vector<Mesh> meshes;
        bool animated = false;
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

        ogl::Texture defaultTexture;
    }; 
} // namespace engine::renderer
