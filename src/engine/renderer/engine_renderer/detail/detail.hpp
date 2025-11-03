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
    struct ProcessedMaterial : public Processed {};

    struct VertexBuffers
    {
        ogl::VBO positions;
        ogl::VBO texCoords;
        ogl::VBO normals;
        ogl::VBO tangents;
        ogl::VBO boneIDs;
        ogl::VBO weights;
    };
    struct Material
    {
        engine::Material::Properties properties;
        struct Textures
        {
            ogl::Texture albedo;
            ogl::Texture normal;
            ogl::Texture metallic;
            ogl::Texture roughness;
        } textures;
    };
    struct Mesh
    {
        ecs::entity e_material;
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

    constexpr unsigned MAX_STORAGE_LIGHTS = 5u;
    struct RendererData
    {
        ogl::Framebuffer oitFBO;
        ogl::Texture oitAccumTexture;
        ogl::Texture oitRevealageTexture;

        ogl::Framebuffer mainFBO;
        ogl::Texture mainFBOColor;
        ogl::Renderbuffer mainFBORBO;

        glm::uvec2 prevCamSize{0};

        ogl::Program screenShader;
        ogl::Program propShader;
        ogl::Program oitCompositeShader;
        ogl::Program skyboxShader;
        ogl::Program depthMapShader;
        ogl::Program depthMapOmnidirectionalShader;

        // The model loader should handle the default textures. This is for edge cases.
        ogl::Texture defaultTexture;

        struct LightsUBOStorage
        {
            struct PointLight
            {
                glm::vec3 color;
                float attenuation;
                glm::vec3 position;
                float farPlane;
            }; // 64 bytes
            struct DirLight
            {
                glm::vec3 direction;
                float _pad0;
                glm::vec3 color;
                float _pad1;
                glm::mat4 viewProj;
            }; // 96 bytes
            struct SpotLight
            {
                glm::vec3 position;
                float innerConeAngle;
                glm::vec3 direction;
                float outerConeAngle;
                glm::vec3 _pad0;
                float attenuation;
                glm::vec3 color;
                float _pad1;
                glm::mat4 viewProj;
            }; // 128 bytes

            uint32_t numPointLights;
            glm::vec3 _pad0;
            std::array<PointLight, MAX_STORAGE_LIGHTS> pointLights;
            uint32_t numDirLights;
            glm::vec3 _pad1;
            std::array<DirLight, MAX_STORAGE_LIGHTS> dirLights;
            uint32_t numSpotLights;
            glm::vec3 _pad2;
            std::array<SpotLight, MAX_STORAGE_LIGHTS> spotLights;
        } lightStorage;
        ogl::UBO lightUBO; 
    }; 
} // namespace engine::renderer
