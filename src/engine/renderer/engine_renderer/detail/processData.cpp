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
void engine::EngineRenderer::processModels(ecs::registry &reg)
{
    using namespace engine;
    for(ecs::entity e_model : reg.view<engine::Model>(ecs::exclude_t<detail::ProcessedModel>{}))
    {
        auto const &model = reg.get<engine::Model>(e_model);
        detail::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            detail::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.material = mesh.material.properties;
            newMesh.count = mesh.primitives.indices.size();
            newMesh.textures = {
                .albedo       = getTexture(reg, mesh.material.textures.albedo),
                .normal       = getTexture(reg, mesh.material.textures.normal),
                .metallic     = getTexture(reg, mesh.material.textures.metallic),
                .roughness     = getTexture(reg, mesh.material.textures.roughness) 
            };
    
            newMesh.buffers = {
                .positions = ogl::makeBuffer<ogl::VBO>(mesh.primitives.positions),
                .texCoords = ogl::makeBuffer<ogl::VBO>(mesh.primitives.texCoords),
                .normals   = ogl::makeBuffer<ogl::VBO>(mesh.primitives.normals),
                .tangents  = ogl::makeBuffer<ogl::VBO>(mesh.primitives.tangents),
                .boneIDs   = ogl::makeBuffer<ogl::VBO>(mesh.primitives.boneIDs),
                .weights   = ogl::makeBuffer<ogl::VBO>(mesh.primitives.weights) 
            };
    
            glCreateVertexArrays(1, &newMesh.vao.id);
            
            addVertexBuffer(newMesh.vao, newMesh.buffers.positions, 4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.texCoords, 2, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.normals,   4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.tangents,  4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.boneIDs,   4, GL_FLOAT);
            addVertexBuffer(newMesh.vao, newMesh.buffers.weights,   4, GL_FLOAT);
            
            newMesh.ibo = ogl::makeBuffer<ogl::IBO>(mesh.primitives.indices);
            glVertexArrayElementBuffer(newMesh.vao.id, newMesh.ibo.id);

            newModel.meshes.emplace_back(std::move(newMesh));
        }

        ecs::entity entity = e_model;
        reg.emplace<detail::Model>(entity, std::move(newModel));
        reg.emplace<detail::ProcessedModel>(e_model, entity);
    }
}
void engine::EngineRenderer::processTextures(ecs::registry &reg)
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
