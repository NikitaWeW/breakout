#pragma once
#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "ogl.hpp"
#include "engine/animation/animation.hpp"

namespace engine::renderer
{
    struct ProcessedModel {};
    struct ProcessedTexture {};
    struct ProcessedCubemap {};
    struct ProcessedMaterial {};

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

    struct PointLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 position;
        float _pad1;
        glm::vec2 depthMapPos;
        float depthMapSize;
        float farPlane;
    };
    struct DirLight
    {
        glm::vec3 color;
        float _pad0;
        glm::vec3 direction;
        float _pad1;
        glm::vec2 depthMapPos;
        float depthMapSize;
        float _pad2;
        glm::vec4 _pad3;
        glm::mat4 viewProj;
    };
    struct SpotLight
    {
        glm::vec3 color;     
        float _pad0;
        glm::vec3 position;  
        float _pad1;
        glm::vec3 direction; 
        float depthMapSize;
        glm::vec2 depthMapPos;
        float innerConeAngle;
        float outerConeAngle;
        glm::mat4 viewProj;
    };

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

        std::vector<renderer::PointLight> pointLights;
        std::vector<renderer::DirLight>   dirLights;
        std::vector<renderer::SpotLight>  spotLights;
        ogl::SSBO pointLightsSSBO; 
        ogl::SSBO dirLightsSSBO; 
        ogl::SSBO spotLightsSSBO; 

        EngineRenderer::Context context;
    }; 
} // namespace engine::renderer
