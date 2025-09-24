#include "Loaders.hpp"
#include "engine/data.hpp"
#include "tiny_obj_loader.h"
#include "engine/loader/loader.hpp"
#include "meshoptimizer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/io.hpp"

namespace engine::loader::detail
{
    static ecs::entity getTexture(ecs::registry &reg, std::string_view path, std::string_view type)
    {
        for(ecs::entity e : reg.view<engine::Texture>())
        {
            auto &texture = reg.get<engine::Texture>(e);

            if(texture.path == path)
            {
                ENGINE_ASSERT_MSG(texture.type == type, "mismatched texture type");
                return e;
            }
        }

        ecs::entity e = engine::loader::load(reg, path);
        reg.get<engine::Texture>(e).type = type;

        return e;
    };
} // namespace engine::loader::detail

ecs::entity engine::loader::detail::ObjModelLoader::load(ecs::registry &reg, std::string_view path)
{
    ENGINE_ASSERT_MSG(reg.view<detail::LoaderData>().size() == 1, "forgot to call engine::loader::setup() / called more than once?");
    loader::detail::LoaderData &data = reg.get<detail::LoaderData>(reg.view<detail::LoaderData>().at(0));
    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = "./";
    tinyobj::ObjReader reader;

    if(!reader.ParseFromFile(std::string{path}, config)) {
        ENGINE_CORE_ERROR("failed to load \"{}\"", path);
        if(!reader.Error().empty()) {
            ENGINE_CORE_ERROR(reader.Error());
        }
        ENGINE_ASSERT_MSG(false, "failed to load a model!");
        return 0;
    }

    if(!reader.Warning().empty()) {
        ENGINE_CORE_WARN(reader.Warning().c_str());
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();

    engine::Mesh unindexed_mesh;
    engine::Model model;
    model.path = path;

    // unfold the index-per-type structure
    for (size_t shape = 0; shape < shapes.size(); shape++) {
        size_t index_offset = 0;
        for (size_t face = 0; face < shapes[shape].mesh.num_face_vertices.size(); face++) {
            size_t fv = size_t(shapes[shape].mesh.num_face_vertices[face]);

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[shape].mesh.indices[index_offset + v];

                unindexed_mesh.positions.emplace_back(
                    attrib.vertices[3*size_t(idx.vertex_index)+0],
                    attrib.vertices[3*size_t(idx.vertex_index)+1],
                    attrib.vertices[3*size_t(idx.vertex_index)+2],
                    1
                );

                if (idx.normal_index >= 0) {
                    unindexed_mesh.normals.emplace_back(
                        attrib.normals[3*size_t(idx.normal_index)+0],
                        attrib.normals[3*size_t(idx.normal_index)+1],
                        attrib.normals[3*size_t(idx.normal_index)+2],
                        0
                    );
                }

                if (idx.texcoord_index >= 0) {
                    unindexed_mesh.texCoords.emplace_back(
                        attrib.texcoords[2*size_t(idx.texcoord_index)+0],
                        attrib.texcoords[2*size_t(idx.texcoord_index)+1] 
                    );
                }
            }
            index_offset += fv;

            // TODO: per-face material
            // shapes[shape].mesh.material_ids[f];
        }
    }

    auto &materials = reader.GetMaterials();
    if(!materials.empty())
    {
        auto const &material = materials[0];
        model.material = {
            .textures = {
                .ambient      = getTexture(reg, material.ambient_texname,      "ao"),
                .diffuse      = getTexture(reg, material.diffuse_texname,      "diffuse"),
                .specular     = getTexture(reg, material.specular_texname,     "specular"),
                .bump         = getTexture(reg, material.bump_texname,         "bump"),
                .displacement = getTexture(reg, material.displacement_texname, "displacement"),
                .alpha        = getTexture(reg, material.alpha_texname,        "alpha"),
                .reflection   = getTexture(reg, material.reflection_texname,   "reflection")
            },
            .ambient = glm::vec3{material.ambient[0], material.ambient[1], material.ambient[2]},
            .diffuse = glm::vec3{material.diffuse[0], material.diffuse[1], material.diffuse[2]},
            .specular = glm::vec3{material.specular[0], material.specular[1], material.specular[2]},
            .transmittance = glm::vec3{material.transmittance[0], material.transmittance[1], material.transmittance[2]},
            .emission = glm::vec3{material.emission[0], material.emission[1], material.emission[2]},
            .shininess = material.shininess,
            .ior = material.ior
        };

    } else
    {
        model.material = data.defaultMaterial;
    }

    if(unindexed_mesh.positions.empty())
    {
        ENGINE_CORE_ERROR("failed to load \"{}\": no positions!", path);
        ENGINE_ASSERT(false);
        return 0;
    }

    if(unindexed_mesh.texCoords.empty())
    {
        std::array<glm::vec2, 4> texCoords{
            glm::vec2{0, 0},
            glm::vec2{1, 0},
            glm::vec2{1, 1},
            glm::vec2{0, 1} 
        };
        // add texture coordinates
        unindexed_mesh.texCoords.reserve(unindexed_mesh.positions.size());
        for(size_t i = 0; i < unindexed_mesh.positions.size(); ++i)
        {
            unindexed_mesh.texCoords.emplace_back(texCoords[i%4]);
        }
    }

    if(unindexed_mesh.normals.empty())
    {
        unindexed_mesh.normals.reserve(unindexed_mesh.positions.size());

        for(size_t i = 0; i < unindexed_mesh.positions.size(); i += 3)
        {
            glm::vec3 e1 = unindexed_mesh.positions[i+1] - unindexed_mesh.positions[i];
            glm::vec3 e2 = unindexed_mesh.positions[i+2] - unindexed_mesh.positions[i];
            glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
            for(unsigned j = 0; j < 3; ++j)
            {
                unindexed_mesh.normals.emplace_back( normal, 0 );
            }
        }
    }

    if(unindexed_mesh.tangents.empty())
    {
        // calculate tangents
        unindexed_mesh.tangents.reserve(unindexed_mesh.positions.size());
        for(size_t i = 0; i < unindexed_mesh.positions.size(); i += 3)
        {
            glm::vec3 edge1 = unindexed_mesh.positions[i+1] - unindexed_mesh.positions[i+0];
            glm::vec3 edge2 = unindexed_mesh.positions[i+2] - unindexed_mesh.positions[i+0];
            glm::vec2 deltaUV1 = unindexed_mesh.texCoords[i+1] - unindexed_mesh.texCoords[i+0];
            glm::vec2 deltaUV2 = unindexed_mesh.texCoords[i+2] - unindexed_mesh.texCoords[i+0]; 
    
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent = {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
            };
            glm::vec3 normal = unindexed_mesh.normals[i];
            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
            for(unsigned j = 0; j < 3; ++j)
            {
                unindexed_mesh.tangents.emplace_back( tangent, 0 );
            }
        }
    }

    ENGINE_ASSERT_MSG(
        unindexed_mesh.positions.size() == unindexed_mesh.normals.size() && 
        unindexed_mesh.positions.size() == unindexed_mesh.texCoords.size() && 
        unindexed_mesh.positions.size() == unindexed_mesh.tangents.size(), 
        "failed to load a model! (messed it up)"
    );

    // remap using meshoptimizer
    size_t index_count = unindexed_mesh.positions.size();
    std::array<meshopt_Stream, 4> streams = {
        meshopt_Stream{&unindexed_mesh.positions[0], sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{&unindexed_mesh.texCoords[0], sizeof(glm::vec2), sizeof(glm::vec2)},
        meshopt_Stream{&unindexed_mesh.normals  [0], sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{&unindexed_mesh.tangents [0], sizeof(glm::vec4), sizeof(glm::vec4)} 
    };
    std::vector<unsigned int> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemapMulti(remap.data(), nullptr, index_count, index_count, streams.data(), streams.size());
    model.mesh.indices.resize(index_count); meshopt_remapIndexBuffer(model.mesh.indices.data(), nullptr, index_count, remap.data());
    model.mesh.positions.resize(vertex_count); meshopt_remapVertexBuffer(model.mesh.positions.data(), streams[0].data, index_count, streams[0].size, remap.data());
    model.mesh.texCoords.resize(vertex_count); meshopt_remapVertexBuffer(model.mesh.texCoords.data(), streams[1].data, index_count, streams[1].size, remap.data());
    model.mesh.normals  .resize(vertex_count); meshopt_remapVertexBuffer(model.mesh.normals  .data(), streams[2].data, index_count, streams[2].size, remap.data());
    model.mesh.tangents .resize(vertex_count); meshopt_remapVertexBuffer(model.mesh.tangents .data(), streams[3].data, index_count, streams[3].size, remap.data());

    return reg.create(std::move(model));
}
