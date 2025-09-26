#include "Loaders.hpp"
#include "engine/data.hpp"
#include "tiny_gltf.h"
#include "engine/loader/loader.hpp"
#include "meshoptimizer.h"
#include <filesystem>

static std::string_view getExtension(std::string_view path)
{
    if(path.find_last_of(".") != std::string::npos)
        return path.substr(path.find_last_of(".") + 1);
    return "";
}
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
static size_t getComponentSize(int componentType) {
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:           return 1;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT:          return 2;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_INT:            return 4;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return 4;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:          return 4;
        default:                                     return 0;
    }
}
static unsigned getNumComponentsInType(int type) {
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR: return 1;
    case TINYGLTF_TYPE_VEC2:   return 2;
    case TINYGLTF_TYPE_VEC3:   return 3;
    case TINYGLTF_TYPE_VEC4:   return 4;
    case TINYGLTF_TYPE_MAT2:   return 4;
    case TINYGLTF_TYPE_MAT3:   return 9;
    case TINYGLTF_TYPE_MAT4:   return 16;
    default:                   return 0;
    }
}
static void addToMesh(engine::Mesh &mesh, tinygltf::Model const &model, tinygltf::Primitive const &primitive)
{
    if(primitive.attributes.find("POSITION") == primitive.attributes.end())
    {
        ENGINE_CORE_ERROR("No positions!");
        return;
    }
    
    {
        tinygltf::Accessor const &accessor = model.accessors.at(primitive.attributes.at("POSITION"));
        ENGINE_ASSERT(accessor.count);
        tinygltf::BufferView const &view = model.bufferViews.at(accessor.bufferView);
        tinygltf::Buffer const &buffer = model.buffers.at(view.buffer);
        float const *data = reinterpret_cast<float const *>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        size_t stride = view.byteStride ? view.byteStride / getComponentSize(accessor.componentType) : getNumComponentsInType(accessor.type);
        for(size_t i = 0; i < accessor.count - (accessor.count % 3); ++i)
        {
            mesh.positions.emplace_back(
                data[i * stride + 0],
                data[i * stride + 1],
                data[i * stride + 2],
                1
            );
        }
    }
    if(primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        tinygltf::Accessor const &accessor = model.accessors.at(primitive.attributes.at("NORMAL"));
        ENGINE_ASSERT(accessor.count);
        tinygltf::BufferView const &view = model.bufferViews.at(accessor.bufferView);
        tinygltf::Buffer const &buffer = model.buffers.at(view.buffer);
        float const *data = reinterpret_cast<float const *>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        size_t stride = view.byteStride ? view.byteStride / getComponentSize(accessor.componentType) : getNumComponentsInType(accessor.type);
        for(size_t i = 0; i < accessor.count - (accessor.count % 3); ++i)
        {
            mesh.normals.emplace_back(
                data[i * stride + 0],
                data[i * stride + 1],
                data[i * stride + 2],
                0
            );
        }
    }
    if(primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
        tinygltf::Accessor const &accessor = model.accessors.at(primitive.attributes.at("TANGENT"));
        ENGINE_ASSERT(accessor.count);
        tinygltf::BufferView const &view = model.bufferViews.at(accessor.bufferView);
        tinygltf::Buffer const &buffer = model.buffers.at(view.buffer);
        float const *data = reinterpret_cast<float const *>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        size_t stride = view.byteStride ? view.byteStride / getComponentSize(accessor.componentType) : getNumComponentsInType(accessor.type);
        for(size_t i = 0; i < accessor.count - (accessor.count % 3); ++i)
        {
            mesh.tangents.emplace_back(
                data[i * stride + 0],
                data[i * stride + 1],
                data[i * stride + 2],
                0
            );
        }
    }
    if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        tinygltf::Accessor const &accessor = model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
        ENGINE_ASSERT(accessor.count);
        tinygltf::BufferView const &view = model.bufferViews.at(accessor.bufferView);
        tinygltf::Buffer const &buffer = model.buffers.at(view.buffer);
        float const *data = reinterpret_cast<float const *>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        size_t stride = view.byteStride ? view.byteStride / getComponentSize(accessor.componentType) : getNumComponentsInType(accessor.type);
        for(size_t i = 0; i < accessor.count - (accessor.count % 3); ++i)
        {
            mesh.texCoords.emplace_back(
                data[i * stride + 0],
                data[i * stride + 1] 
            );
        }
    }
    if(primitive.attributes.find("JOINTS_0") != primitive.attributes.end() && 
       primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
    {
        tinygltf::Accessor const &wAccessor = model.accessors.at(primitive.attributes.at("WEIGHTS_0"));
        ENGINE_ASSERT(wAccessor.count);
        tinygltf::BufferView const &wView = model.bufferViews.at(wAccessor.bufferView);
        tinygltf::Buffer const &wBuffer = model.buffers.at(wView.buffer);
        float const *wData = reinterpret_cast<float const *>(wBuffer.data.data() + wView.byteOffset + wAccessor.byteOffset);
        size_t wStride = wView.byteStride ? wView.byteStride / getComponentSize(wAccessor.type) : getNumComponentsInType(wAccessor.type);

        tinygltf::Accessor const &jAccessor = model.accessors.at(primitive.attributes.at("JOINTS_0"));
        ENGINE_ASSERT(jAccessor.count);
        tinygltf::BufferView const &jView = model.bufferViews.at(jAccessor.bufferView);
        tinygltf::Buffer const &jBuffer = model.buffers.at(jView.buffer);
        float const *jData = reinterpret_cast<float const *>(jBuffer.data.data() + jView.byteOffset + jAccessor.byteOffset);
        size_t jStride = jView.byteStride ? jView.byteStride / getComponentSize(jAccessor.type) : getNumComponentsInType(jAccessor.type);

        for(size_t i = 0; i < glm::min((size_t)jAccessor.count - (jAccessor.count % 3), (size_t)wAccessor.count - (wAccessor.count % 3)); ++i)
        {
            mesh.weights.emplace_back(
                wData[i * wStride + 0],
                wData[i * wStride + 1],
                wData[i * wStride + 2],
                wData[i * wStride + 3] 
            );
            mesh.boneIDs.emplace_back(
                jData[i * jStride + 0],
                jData[i * jStride + 1],
                jData[i * jStride + 2],
                jData[i * jStride + 3] 
            );
        }
    }
    if (primitive.indices >= 0) {
        tinygltf::Accessor const &accessor = model.accessors.at(primitive.indices);
        ENGINE_ASSERT(accessor.count);
        tinygltf::BufferView const &view = model.bufferViews.at(accessor.bufferView);
        tinygltf::Buffer const &buffer = model.buffers.at(view.buffer);
        unsigned const *data = reinterpret_cast<unsigned const *>(buffer.data.data() + view.byteOffset + accessor.byteOffset);
        size_t stride = view.byteStride ? view.byteStride / getComponentSize(accessor.componentType) : getNumComponentsInType(accessor.type);
        for(size_t i = 0; i < accessor.count - (accessor.count % 3); ++i)
        {
            mesh.indices.emplace_back(
                data[i * stride]
            );
        }
    }
}

ecs::entity engine::loader::detail::GLTFModelLoader::load(ecs::registry &reg, std::string_view path)
{
    ENGINE_ASSERT_MSG(reg.view<detail::LoaderData>().size() == 1, "forgot to call engine::loader::setup() / called more than once?");
    loader::detail::LoaderData &data = reg.get<detail::LoaderData>(reg.view<detail::LoaderData>().at(0));

    using namespace tinygltf;

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool success = false;

    if(getExtension(path) == "glb")
        success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path.data());
    else
        success = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path.data());

    if (!warn.empty())
        ENGINE_CORE_WARN(warn);
    if (!err.empty())
        ENGINE_CORE_ERROR(err);
    if (!success) {
        ENGINE_CORE_ERROR("Failed to parse glTF model");
        return 0;
    }

    Model model;

    if(gltfModel.meshes.empty())
    {
        ENGINE_CORE_ERROR("No meshes!");
        return 0;
    }
    for(tinygltf::Mesh const &mesh : gltfModel.meshes)
    {
        for(tinygltf::Primitive const &primitive : mesh.primitives)
        {
            addToMesh(model.mesh, gltfModel, primitive);
        }
    }

    calculateMissingPrimitives(model.mesh);


    if(!gltfModel.materials.empty())
    { 
        // TODO: multiple materials (multiple meshes)
        // FIXME TODO: fill material here
        model.mesh.material = data.defaultMaterial;
    } else
    {
        model.mesh.material = data.defaultMaterial;
    }

    return reg.create(std::move(model));
}
