#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "detail.hpp"
#include "ogl.hpp"

namespace ogl = engine::renderer::ogl;

static ogl::Texture getTexture(ecs::registry const &reg, ecs::entity e_texture)
{
    using namespace engine;
    if(e_texture == 0)
        return ogl::Texture{};
    ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedTexture>(e_texture), "Forgot to call engine::EngineRenderer::processData?");

    return reg.get<ogl::Texture>(e_texture);
}
static void addVertexBuffer(ogl::VAO &vao, ogl::Buffer &buff, std::size_t count, GLenum type)
{
    using namespace engine;
    if(buff.id == 0) 
    {
        ++vao.numVertexBuffers;
        return;
    }
    unsigned attrib = vao.numVertexBuffers;
    glVertexArrayVertexBuffer(vao.id, vao.numVertexBuffers, buff.id, 0, count * ogl::getSizeOfGLType(type));
    glEnableVertexArrayAttrib(vao.id, attrib);
    switch(type)
    {
    case GL_INT:
        glVertexArrayAttribIFormat(vao.id, attrib, count, type, 0);
        break;
    case GL_UNSIGNED_INT:
        glVertexArrayAttribIFormat(vao.id, attrib, count, type, 0);
        break;
    default:
        glVertexArrayAttribFormat(vao.id, attrib, count, type, GL_FALSE, 0);
        break;
    }
    glVertexArrayAttribBinding(vao.id, attrib, vao.numVertexBuffers);
    ++vao.numVertexBuffers;
}
void engine::EngineRenderer::processModels(ecs::registry &reg)
{
    using namespace engine;
    for(ecs::entity e_model : reg.view<engine::Model>(ecs::exclude_t<renderer::ProcessedModel>{}))
    {
        auto const &model = reg.get<engine::Model>(e_model);
        renderer::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedMaterial>(mesh.e_material), "Forgot to call engine::EngineRenderer::processData?");
            renderer::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.count = mesh.geometry.indices.size();
    
            newMesh.e_material = mesh.e_material;
            newMesh.buffers = {
                .positions = ogl::makeBuffer<ogl::VBO>(mesh.geometry.positions),
                .texCoords = ogl::makeBuffer<ogl::VBO>(mesh.geometry.texCoords),
                .normals   = ogl::makeBuffer<ogl::VBO>(mesh.geometry.normals),
                .tangents  = ogl::makeBuffer<ogl::VBO>(mesh.geometry.tangents),
                .boneIDs   = ogl::makeBuffer<ogl::VBO>(mesh.geometry.boneIDs),
                .weights   = ogl::makeBuffer<ogl::VBO>(mesh.geometry.weights) 
            };
    
            glCreateVertexArrays(1, &newMesh.vao.id);
            
            addVertexBuffer(newMesh.vao, newMesh.buffers.positions, 4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.texCoords, 2, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.normals,   4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.tangents,  4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.boneIDs,   4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.weights,   4, GL_FLOAT);
            
            newMesh.ibo = ogl::makeBuffer<ogl::IBO>(mesh.geometry.indices);
            glVertexArrayElementBuffer(newMesh.vao.id, newMesh.ibo.id);

            newModel.meshes.emplace_back(std::move(newMesh));
        }

        reg.emplace<renderer::Model>(e_model, std::move(newModel));
        reg.emplace<renderer::ProcessedModel>(e_model);
    }
}
void engine::EngineRenderer::processMaterials(ecs::registry &reg) 
{
    for(ecs::entity e_material : reg.view<engine::Material>(ecs::exclude_t<renderer::ProcessedMaterial>{}))
    {
        auto const &material = reg.get<engine::Material>(e_material);
        ecs::entity const &entity = e_material; // destination

        renderer::Material newMaterial;
        newMaterial.properties = material.properties;
        newMaterial.textures = {
            .albedo    = getTexture(reg, material.textures.albedo),
            .normal    = getTexture(reg, material.textures.normal),
            .metallic  = getTexture(reg, material.textures.metallic),
            .roughness = getTexture(reg, material.textures.roughness) 
        };

        reg.emplace<renderer::Material>(entity, std::move(newMaterial));
        reg.emplace<renderer::ProcessedMaterial>(e_material);
    }
}
void engine::EngineRenderer::processTextures(ecs::registry &reg)
{
    for(ecs::entity e_texture : reg.view<engine::Texture>(ecs::exclude_t<renderer::ProcessedTexture>{}))
    {
        auto const &texture = reg.get<engine::Texture>(e_texture);

        reg.emplace<ogl::Texture>(e_texture, ogl::makeTexture(texture.data, texture.srgb));
        reg.emplace<renderer::ProcessedTexture>(e_texture);
    }

    for(ecs::entity e_cubemap : reg.view<engine::Cubemap>(ecs::exclude_t<renderer::ProcessedCubemap>{}))
    {
        auto const &cubemap = reg.get<engine::Cubemap>(e_cubemap);

        reg.emplace<ogl::Cubemap>(e_cubemap, ogl::makeCubemap(cubemap.faces));
        reg.emplace<renderer::ProcessedCubemap>(e_cubemap);
    }
}

static void processPointLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::PointLight>(e_light);
    data.pointLights.emplace_back(engine::renderer::PointLight{
        .color = light.color,
        .position = reg.has<engine::ModelMatrix>(e_light) ? reg.get<engine::ModelMatrix>(e_light)[3] : glm::vec3{0},
        .depthMapPos = glm::vec2{0},
        .depthMapSize = 0.0f,
        .farPlane = 0.0f,
    });
}
static void processDirLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::DirectionalLight>(e_light);
    glm::mat4 const &modelMat = reg.has<engine::ModelMatrix>(e_light) ? static_cast<glm::mat4 const &>(reg.get<engine::ModelMatrix>(e_light)) : glm::mat4{1.0f};
    data.dirLights.emplace_back(engine::renderer::DirLight{
        .color = light.color,
        .direction = -glm::normalize(glm::vec3(modelMat[2])),
        .depthMapPos = glm::vec2{0},
        .depthMapSize = 0.0f,
        .viewProj = modelMat
    });
}
static void processSpotLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::SpotLight>(e_light);
    glm::mat4 const &modelMat = reg.has<engine::ModelMatrix>(e_light) ? static_cast<glm::mat4 const &>(reg.get<engine::ModelMatrix>(e_light)) : glm::mat4{1.0f};
    data.spotLights.emplace_back(engine::renderer::SpotLight{
        .color = light.color,
        .position = modelMat[3],
        .direction = -glm::normalize(glm::vec3(modelMat[2])),
        .depthMapSize = 0.0f,
        .depthMapPos = glm::vec2{0},
        .innerConeAngle = light.innerConeAngle,
        .outerConeAngle = light.outerConeAngle,
        .viewProj = modelMat,
    });
}
void engine::EngineRenderer::processLights(ecs::registry const &reg, renderer::RendererData &data)
{
    data.pointLights.clear();
    data.dirLights.clear();
    data.spotLights.clear();
    for(ecs::entity e_light : reg.view<DynamicLight>())
    {
        if(reg.has<PointLight>(e_light))
            processPointLight(data, reg, e_light);
        if(reg.has<DirectionalLight>(e_light))
            processDirLight(data, reg, e_light);
        if(reg.has<SpotLight>(e_light))
            processSpotLight(data, reg, e_light);
        if(reg.has<AreaLight>(e_light))
            ENGINE_ASSERT_MSG(false, "Area lights not implemented yet!");
    } 

    glNamedBufferData(data.pointLightsSSBO.id, data.pointLights.size() * sizeof(renderer::PointLight), data.pointLights.data(), GL_STATIC_DRAW);
    glNamedBufferData(data.dirLightsSSBO.id,   data.dirLights.size()   * sizeof(renderer::DirLight),   data.dirLights.data(),   GL_STATIC_DRAW);
    glNamedBufferData(data.spotLightsSSBO.id,  data.spotLights.size()  * sizeof(renderer::SpotLight),  data.spotLights.data(),  GL_STATIC_DRAW);
}

void engine::EngineRenderer::processData(ecs::registry &reg)
{
    processTextures(reg);
    processMaterials(reg);
    processModels(reg);
}
