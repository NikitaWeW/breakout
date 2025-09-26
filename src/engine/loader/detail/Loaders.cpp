#include "Loaders.hpp"

void engine::loader::detail::calculateMissingPrimitives(engine::Mesh &mesh)
{
    ENGINE_ASSERT(!mesh.positions.empty());
    ENGINE_ASSERT(mesh.positions.size() % 3 == 0);

    if(mesh.texCoords.empty())
    {
        std::array<glm::vec2, 4> texCoords{
            glm::vec2{0, 0},
            glm::vec2{1, 0},
            glm::vec2{1, 1},
            glm::vec2{0, 1} 
        };
        // add texture coordinates
        mesh.texCoords.reserve(mesh.positions.size());
        for(size_t i = 0; i < mesh.positions.size(); ++i)
        {
            mesh.texCoords.emplace_back(texCoords[i%4]);
        }
    }

    if(mesh.normals.empty())
    {
        mesh.normals.reserve(mesh.positions.size());

        for(size_t i = 0; i < mesh.positions.size(); i += 3)
        {
            glm::vec3 e1 = mesh.positions[i+1] - mesh.positions[i];
            glm::vec3 e2 = mesh.positions[i+2] - mesh.positions[i];
            glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
            for(unsigned j = 0; j < 3; ++j)
            {
                mesh.normals.emplace_back( normal, 0 );
            }
        }
    }

    if(mesh.tangents.empty())
    {
        // calculate tangents
        mesh.tangents.reserve(mesh.positions.size());
        for(size_t i = 0; i < mesh.positions.size(); i += 3)
        {
            glm::vec3 edge1 = mesh.positions[i+1] - mesh.positions[i+0];
            glm::vec3 edge2 = mesh.positions[i+2] - mesh.positions[i+0];
            glm::vec2 deltaUV1 = mesh.texCoords[i+1] - mesh.texCoords[i+0];
            glm::vec2 deltaUV2 = mesh.texCoords[i+2] - mesh.texCoords[i+0]; 
    
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent = {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
            };
            glm::vec3 normal = mesh.normals[i];
            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
            for(unsigned j = 0; j < 3; ++j)
            {
                mesh.tangents.emplace_back( tangent, 0 );
            }
        }
    }

    ENGINE_ASSERT(
        mesh.positions.size() == mesh.normals.size() && 
        mesh.positions.size() == mesh.texCoords.size() && 
        mesh.positions.size() == mesh.tangents.size()
    );
}