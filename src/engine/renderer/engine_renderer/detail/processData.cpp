#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "detail.hpp"
#include "ogl.hpp"

static ogl::Texture getTexture(ecs::registry const &reg, ecs::entity e_texture)
{
    using namespace engine;
    if(e_texture == 0)
        return ogl::Texture{};
    ENGINE_ASSERT(reg.has<detail::ProcessedTexture>(e_texture));

    return reg.get<ogl::Texture>(reg.get<detail::ProcessedTexture>(e_texture).data);
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
static void processModels(ecs::registry &reg)
{
    using namespace engine;
    for(ecs::entity e_model : reg.view<engine::Model>(ecs::exclude_t<detail::ProcessedModel>{}))
    {
        auto const &model = reg.get<engine::Model>(e_model);
        detail::Model newModel;

        for(auto const &mesh : model.meshes)
        {
            detail::Mesh newMesh;
            newMesh.animated = !mesh.skeleton.weights.empty();
            newMesh.mode = GL_TRIANGLES;
            newMesh.material = mesh.material.properties;
            newMesh.skeleton = mesh.skeleton;
            newMesh.count = mesh.indices.size();
            newMesh.textures = {
                .ambient      = getTexture(reg, mesh.material.textures.ambient),
                .albedo       = getTexture(reg, mesh.material.textures.albedo),
                .specular     = getTexture(reg, mesh.material.textures.specular),
                .normal       = getTexture(reg, mesh.material.textures.normal),
                .displacement = getTexture(reg, mesh.material.textures.displacement),
                .alpha        = getTexture(reg, mesh.material.textures.alpha),
                .reflection   = getTexture(reg, mesh.material.textures.reflection),
                .metallic     = getTexture(reg, mesh.material.textures.metallic) 
            };
    
            newMesh.buffers = {
                .positions = ogl::makeBuffer<ogl::VBO>(mesh.positions),
                .texCoords = ogl::makeBuffer<ogl::VBO>(mesh.texCoords),
                .normals   = ogl::makeBuffer<ogl::VBO>(mesh.normals),
                .tangents  = ogl::makeBuffer<ogl::VBO>(mesh.tangents),
                .boneIDs   = ogl::makeBuffer<ogl::VBO>(mesh.skeleton.boneIDs),
                .weights   = ogl::makeBuffer<ogl::VBO>(mesh.skeleton.weights) 
            };
    
            glCreateVertexArrays(1, &newMesh.vao.id);
            
            addVertexBuffer(newMesh.vao, newMesh.buffers.positions, 4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.texCoords, 2, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.normals,   4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.tangents,  4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.boneIDs,   4, GL_INT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.weights,   4, GL_FLOAT);
            
            newMesh.ibo = ogl::makeBuffer<ogl::IBO>(mesh.indices);
            glVertexArrayElementBuffer(newMesh.vao.id, newMesh.ibo.id);

            newModel.meshes.emplace_back(std::move(newMesh));
        }

        ecs::entity entity = e_model;
        reg.emplace<detail::Model>(entity, std::move(newModel));
        reg.emplace<detail::ProcessedModel>(e_model, entity);
    }
}
static void processTextures(ecs::registry &reg)
{
    using namespace engine;
    for(ecs::entity e_texture : reg.view<engine::Texture>(ecs::exclude_t<detail::ProcessedTexture>{}))
    {
        auto const &texture = reg.get<engine::Texture>(e_texture);

        ecs::entity const &entity = e_texture;

        reg.emplace<ogl::Texture>(entity, ogl::makeTexture(texture.data, texture.srgb));
        reg.emplace<detail::ProcessedTexture>(e_texture, entity);
    }
}


void engine::EngineRenderer::processData(ecs::registry &reg)
{
    processTextures(reg);
    processModels(reg);
}
