#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "detail.hpp"
#include "ogl.hpp"

static ogl::Texture const &getTexture(ecs::registry const &reg, ecs::entity e_texture)
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

        detail::Mesh mesh;
        mesh.animated = !model.mesh.weights.empty();
        mesh.mode = GL_TRIANGLES;
        mesh.material = model.mesh.material.properties;
        mesh.count = model.mesh.indices.size();
        mesh.textures = {
            .ambient      = getTexture(reg, model.mesh.material.textures.ambient),
            .diffuse      = getTexture(reg, model.mesh.material.textures.diffuse),
            .specular     = getTexture(reg, model.mesh.material.textures.specular),
            .bump         = getTexture(reg, model.mesh.material.textures.bump),
            .displacement = getTexture(reg, model.mesh.material.textures.displacement),
            .alpha        = getTexture(reg, model.mesh.material.textures.alpha),
            .reflection   = getTexture(reg, model.mesh.material.textures.reflection) 
        };

        mesh.buffers = {
            .positions = ogl::makeBuffer<ogl::VBO>(model.mesh.positions),
            .texCoords = ogl::makeBuffer<ogl::VBO>(model.mesh.texCoords),
            .normals   = ogl::makeBuffer<ogl::VBO>(model.mesh.normals),
            .tangents  = ogl::makeBuffer<ogl::VBO>(model.mesh.tangents),
            .boneIDs   = ogl::makeBuffer<ogl::VBO>(model.mesh.boneIDs),
            .weights   = ogl::makeBuffer<ogl::VBO>(model.mesh.weights) 
        };

        glCreateVertexArrays(1, &mesh.vao.id);
        
        addVertexBuffer(mesh.vao, mesh.buffers.positions, 4, GL_FLOAT);
        addVertexBuffer(mesh.vao, mesh.buffers.texCoords, 2, GL_FLOAT);
        addVertexBuffer(mesh.vao, mesh.buffers.normals,   4, GL_FLOAT);
        addVertexBuffer(mesh.vao, mesh.buffers.tangents,  4, GL_FLOAT);
        addVertexBuffer(mesh.vao, mesh.buffers.boneIDs,   4, GL_INT);
        addVertexBuffer(mesh.vao, mesh.buffers.weights,   4, GL_FLOAT);
        
        mesh.ibo = ogl::makeBuffer<ogl::IBO>(model.mesh.indices);
        glVertexArrayElementBuffer(mesh.vao.id, mesh.ibo.id);

        ecs::entity entity = e_model;
        reg.emplace<detail::Mesh>(entity, std::move(mesh));
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
