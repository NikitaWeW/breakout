#include "Loaders.hpp"

void engine::loader::detail::calculateMissingPrimitives(engine::Mesh &mesh)
{
    ENGINE_ASSERT(!mesh.positions.empty());

    bool indexed = !mesh.indices.empty();
    if(mesh.texCoords.empty())
    {
        ENGINE_CORE_TRACE("calculating texcoords");
        mesh.texCoords.resize(mesh.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.indices.size() : mesh.positions.size()); i+=3)
        {
            unsigned index = indexed ? mesh.indices[i] : i;

            mesh.texCoords[index] = std::array<glm::vec2, 4>{
                glm::vec2{0, 0},
                glm::vec2{1, 0},
                glm::vec2{1, 1},
                glm::vec2{0, 1} 
            }[index%4];
        }
    }

    if(mesh.normals.empty())
    {
        ENGINE_CORE_TRACE("calculating normals");
        mesh.normals.resize(mesh.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.indices.size() : mesh.positions.size()); i+=3)
        {
            glm::vec3 e1 = mesh.positions[indexed ? mesh.indices[i+1] : i+1] - mesh.positions[indexed ? mesh.indices[i+0] : i+0];
            glm::vec3 e2 = mesh.positions[indexed ? mesh.indices[i+2] : i+2] - mesh.positions[indexed ? mesh.indices[i+0] : i+0];
            glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
            for(unsigned j = 0; j < 3; ++j)
                mesh.normals[indexed ? mesh.indices[i+0] : i+0] = { normal, 0 };
        }
    }

    if(mesh.tangents.empty())
    {
        ENGINE_CORE_TRACE("calculating tangents");
        mesh.tangents.resize(mesh.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.indices.size() : mesh.positions.size()); i+=3)
        {
            unsigned index = indexed ? mesh.indices[i] : i;
            glm::vec3 edge1 = mesh.positions[indexed ? mesh.indices[i+1] : i+1] - mesh.positions[index];
            glm::vec3 edge2 = mesh.positions[indexed ? mesh.indices[i+2] : i+2] - mesh.positions[index];
            glm::vec2 deltaUV1 = mesh.texCoords[indexed ? mesh.indices[i+1] : i+1] - mesh.texCoords[index];
            glm::vec2 deltaUV2 = mesh.texCoords[indexed ? mesh.indices[i+2] : i+2] - mesh.texCoords[index]; 

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent = {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
            };
            glm::vec3 normal = mesh.normals[index];
            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
            for(unsigned j = 0; j < 3; ++j)
                mesh.tangents[index] = { tangent, 0 };
        }
    }
}

#include "meshoptimizer.h"
void engine::loader::detail::optimizeMesh(engine::Mesh &mesh)
{
    engine::Mesh oldMesh = mesh;

    bool indexed = !oldMesh.indices.empty();

    size_t index_count = indexed ? oldMesh.indices.size() : oldMesh.positions.size();
    size_t vertex_count = indexed ? oldMesh.positions.size() : index_count;
    std::vector<meshopt_Stream> streams = {
        meshopt_Stream{oldMesh.positions.data(), sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{oldMesh.texCoords.data(), sizeof(glm::vec2), sizeof(glm::vec2)},
        meshopt_Stream{oldMesh.normals  .data(), sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{oldMesh.tangents .data(), sizeof(glm::vec4), sizeof(glm::vec4)} 
    };

    if(!oldMesh.boneIDs.empty())
    {
        streams.emplace_back(meshopt_Stream{oldMesh.boneIDs  .data(), sizeof(glm::ivec4), sizeof(glm::ivec4)});
        streams.emplace_back(meshopt_Stream{oldMesh.weights  .data(), sizeof(glm::vec4),  sizeof(glm::vec4)});
    }

    std::vector<unsigned int> remap(vertex_count);
    size_t new_vertex_count = meshopt_generateVertexRemapMulti(remap.data(), indexed ? oldMesh.indices.data() : nullptr, index_count, vertex_count, streams.data(), streams.size());
    mesh.indices.resize(index_count); meshopt_remapIndexBuffer(mesh.indices.data(), indexed ? oldMesh.indices.data() : nullptr, index_count, remap.data());
    mesh.positions.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.positions.data(), streams[0].data, vertex_count, streams[0].size, remap.data());
    mesh.texCoords.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.texCoords.data(), streams[1].data, vertex_count, streams[1].size, remap.data());
    mesh.normals  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.normals  .data(), streams[2].data, vertex_count, streams[2].size, remap.data());
    mesh.tangents .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.tangents .data(), streams[3].data, vertex_count, streams[3].size, remap.data());
    if(!oldMesh.boneIDs.empty())
    {
        mesh.boneIDs  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.boneIDs  .data(), streams[4].data, vertex_count, streams[4].size, remap.data());
        mesh.weights  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.weights  .data(), streams[5].data, vertex_count, streams[5].size, remap.data());
    }
}
