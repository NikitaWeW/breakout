#include "Loaders.hpp"
#include "engine/data.hpp"
#include "tiny_obj_loader.h"
#include "engine/loader/loader.hpp"
#include <filesystem>

static ecs::entity getTexture(ecs::registry &reg, std::string_view path, ecs::entity defaultTexture, bool srgb)
{
    if(path == "")
        return defaultTexture;
    for(ecs::entity e : reg.view<engine::Texture>())
    {
        auto &texture = reg.get<engine::Texture>(e);

        if(texture.path == path)
        {
            return e;
        }
    }

    ecs::entity e = engine::loader::load(reg, path);
    reg.get<engine::Texture>(e).srgb = srgb;

    return e;
};

ecs::entity engine::loader::detail::ObjModelLoader::load(ecs::registry &reg, std::string_view path)
{
    ENGINE_ASSERT_MSG(reg.view<detail::LoaderData>().size() == 1, "forgot to call engine::loader::setup() / called more than once?");
    loader::detail::LoaderData &data = reg.get<detail::LoaderData>(reg.view<detail::LoaderData>().at(0));
    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = std::filesystem::path{path}.parent_path();
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

    engine::Model model;
    model.path = path;

    // unfold the index-per-type structure
    for (size_t shape = 0; shape < shapes.size(); shape++) {
        size_t index_offset = 0;
        for (size_t face = 0; face < shapes[shape].mesh.num_face_vertices.size(); face++) {
            size_t fv = size_t(shapes[shape].mesh.num_face_vertices[face]);

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[shape].mesh.indices[index_offset + v];

                model.mesh.positions.emplace_back(
                    attrib.vertices[3*size_t(idx.vertex_index)+0],
                    attrib.vertices[3*size_t(idx.vertex_index)+1],
                    attrib.vertices[3*size_t(idx.vertex_index)+2],
                    1
                );

                if (idx.normal_index >= 0) {
                    model.mesh.normals.emplace_back(
                        attrib.normals[3*size_t(idx.normal_index)+0],
                        attrib.normals[3*size_t(idx.normal_index)+1],
                        attrib.normals[3*size_t(idx.normal_index)+2],
                        0
                    );
                }

                if (idx.texcoord_index >= 0) {
                    model.mesh.texCoords.emplace_back(
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
        model.mesh.material = {
            .textures = {
                .ambient      = getTexture(reg, material.ambient_texname,      data.defaultMaterial.textures.ambient,      false),
                .diffuse      = getTexture(reg, material.diffuse_texname,      data.defaultMaterial.textures.diffuse,      true ),
                .specular     = getTexture(reg, material.specular_texname,     data.defaultMaterial.textures.specular,     false),
                .bump         = getTexture(reg, material.bump_texname,         data.defaultMaterial.textures.bump,         false),
                .displacement = getTexture(reg, material.displacement_texname, data.defaultMaterial.textures.displacement, false),
                .alpha        = getTexture(reg, material.alpha_texname,        data.defaultMaterial.textures.alpha,        false),
                .reflection   = getTexture(reg, material.reflection_texname,   data.defaultMaterial.textures.reflection,   false) 
            },
            .properties = {
                .ambient = glm::vec3{material.ambient[0], material.ambient[1], material.ambient[2]},
                .diffuse = glm::vec3{material.diffuse[0], material.diffuse[1], material.diffuse[2]},
                .specular = glm::vec3{material.specular[0], material.specular[1], material.specular[2]},
                .transmittance = glm::vec3{material.transmittance[0], material.transmittance[1], material.transmittance[2]},
                .emission = glm::vec3{material.emission[0], material.emission[1], material.emission[2]},
                .shininess = material.shininess,
                .ior = material.ior
            }
        };

    } else
    {
        model.mesh.material = data.defaultMaterial;
    }

    if(model.mesh.positions.empty())
    {
        ENGINE_CORE_ERROR("failed to load \"{}\": no positions!", path);
        ENGINE_ASSERT(false);
        return 0;
    }

    calculateMissingPrimitives(model.mesh);
    optimizeMesh(model.mesh);

    return reg.create(std::move(model));
}
