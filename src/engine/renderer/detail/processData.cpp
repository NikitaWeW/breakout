#include "engine/renderer/renderer.hpp"
#include "engine/data.hpp"
#include "mesh.hpp"

namespace engine::renderer::detail
{
    static ogl::Object getTexture(ecs::registry const &reg, ecs::entity e_texture)
    {
        ENGINE_ASSERT(reg.has<renderer::Processed>(e_texture), "unprocessed texture!");

        return reg.get<ogl::Object>(reg.get<renderer::Processed>(e_texture).data);
    }
    static void addVertexBuffer(ogl::VAO vao, ogl::Buffer buff, std::size_t count, GLenum type)
    {
        glVertexArrayVertexBuffer(vao.id, vao.numVertexBuffers, buff.id, 0, buff.size);
        glEnableVertexArrayAttrib(vao.id, vao.numVertexBuffers);
        glVertexArrayAttribFormat(vao.id, vao.numVertexBuffers, count, type, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao.id, vao.numVertexBuffers, vao.numVertexBuffers);
        ++vao.numVertexBuffers;
    }
    static void processModels(ecs::registry &reg)
    {
        for(ecs::entity e_model : reg.view<engine::Model>())
        {
            auto const &model = reg.get<engine::Model>(e_model);

            detail::Mesh mesh;
            mesh.animated = !model.mesh.weights.empty();
            mesh.mode = GL_TRIANGLES;
            mesh.material = model.material;
            mesh.textures = {
                .ambient      = getTexture(reg, model.textures.ambient),
                .diffuse      = getTexture(reg, model.textures.diffuse),
                .specular     = getTexture(reg, model.textures.specular),
                .bump         = getTexture(reg, model.textures.bump),
                .displacement = getTexture(reg, model.textures.displacement),
                .alpha        = getTexture(reg, model.textures.alpha),
                .reflection   = getTexture(reg, model.textures.reflection) 
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
            addVertexBuffer(mesh.vao, mesh.buffers.boneIDs,   4, GL_FLOAT);
            addVertexBuffer(mesh.vao, mesh.buffers.weights,   4, GL_FLOAT);
            
            mesh.ibo = ogl::makeBuffer<ogl::IBO>(model.mesh.indices);
            glVertexArrayElementBuffer(mesh.vao.id, mesh.ibo.id);
        }
    }
    static void processTextures(ecs::registry &reg)
    {
        for(ecs::entity e_texture : reg.view<engine::Texture>())
        {
            auto const &texture = reg.get<engine::Texture>(e_texture);

            ecs::entity const &entity = e_texture;

            reg.emplace<ogl::Object>(entity, ogl::makeTexture(texture.data, texture.type == "diffuse"));
            reg.emplace<renderer::ProcessedTexture>(e_texture, entity);
        }
    }
} // namespace engine::renderer::detail


void engine::renderer::detail::processData(ecs::registry &reg)
{
    detail::processTextures(reg);
    detail::processModels(reg);
}
