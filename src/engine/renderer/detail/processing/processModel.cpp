#include "process.hpp"
#include "../ogl/VertexBuffer.hpp"
#include "../ogl/IndexBuffer.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/config.hpp"

namespace engine::renderer::detail
{
    detail::Mesh createMesh(engine::Model const &model)
    {
        detail::Mesh mesh;

        mesh.material = model.material;
        mesh.textures = model.textures;

        engine::Mesh const &data = model.mesh;
        
        mesh.vbo = ogl::VertexBuffer{
            data.positions.size()     * sizeof(data.positions[0]) + 
            data.normals.size()       * sizeof(data.normals[0]) + 
            data.texCoords.size() * sizeof(data.texCoords[0]) + 
            data.tangents.size()      * sizeof(data.tangents[0]) +
            data.boneIDs.size() * sizeof(data.boneIDs[0]) +
            data.weights.size() * sizeof(data.weights[0])
        };
        glBufferSubData(GL_ARRAY_BUFFER, 
            0, 
            data.positions.size() * sizeof(data.positions[0]), 
            data.positions.data());
        glBufferSubData(GL_ARRAY_BUFFER, 
            data.positions.size() * sizeof(data.positions[0]), 
            data.normals.size() * sizeof(data.normals[0]), 
            data.normals.data());
        glBufferSubData(GL_ARRAY_BUFFER, 
            data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]), 
            data.texCoords.size() * sizeof(data.texCoords[0]), 
            data.texCoords.data());
        glBufferSubData(GL_ARRAY_BUFFER, 
            data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0]), 
            data.tangents.size() * sizeof(data.tangents[0]), 
            data.tangents.data());
        if(!data.boneIDs.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, 
                data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]), 
                data.boneIDs.size() * sizeof(data.boneIDs[0]), 
                data.boneIDs.data());
            glBufferSubData(GL_ARRAY_BUFFER, 
                data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]) + data.boneIDs.size() * sizeof(data.boneIDs[0]), 
                data.weights.size() * sizeof(data.weights[0]), 
                data.weights.data());
        }

        mesh.ibo = ogl::IndexBuffer{data.indices.size() * sizeof(data.indices[0]), data.indices.data()};
        mesh.vao = ogl::VertexArray{mesh.vbo, ogl::VertexBufferLayout{
            {4, GL_FLOAT, 0},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0])},
            {2, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0])},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0])},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0]) + data.tangents.size() * sizeof(data.tangents[0])},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.texCoords.size() * sizeof(data.texCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]) + data.boneIDs.size() * sizeof(data.boneIDs[0])},
        }};
        mesh.count = static_cast<unsigned>(data.indices.size());

        return mesh;
    }

    void processModels(ecs::registry &reg)
    {
        ENGINE_PROFILE()
        auto models = reg.view<Model>(ecs::exclude_t<Processed>{});
    
        for(auto const &e : models)
        {
            auto const &model = reg.get<Model>(e);
            auto const &meshEntity = e;

            ENGINE_ASSERT(model.mesh.positions.size(), "Empty model: no positions supplied!");
            ENGINE_ASSERT(model.mesh.normals.size(),   "Empty model: no normals supplied!");
            ENGINE_ASSERT(model.mesh.texCoords.size(), "Empty model: no texture coordinates supplied!");
            ENGINE_ASSERT(model.mesh.tangents.size(),  "Empty model: no tangents supplied!");
    
            reg.emplace<detail::Mesh>(meshEntity, createMesh(model));
            reg.emplace<Processed>(e, meshEntity);
        }
    }
} // namespace engine::renderer::detail
