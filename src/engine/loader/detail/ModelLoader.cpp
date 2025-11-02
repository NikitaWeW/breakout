#include "Loaders.hpp"
#include <filesystem>

#include "meshoptimizer.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "stb_image.h"

// Enable verbose loading by uncommenting the line below or adding a compiler definition.
// #define MODEL_LOADER_TRACE(...) ENGINE_CORE_TRACE(__VA_ARGS__)
#ifndef MODEL_LOADER_TRACE
#define MODEL_LOADER_TRACE(...)
#endif

constexpr glm::mat4 toMat4(aiMatrix4x4 const &from)
{
    glm::mat4 to{};
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}
template<typename aiVector3X> 
constexpr glm::vec3 toVec3(aiVector3X const &aivector)
{
    return glm::vec3{(float) aivector.x, (float) aivector.y, (float) aivector.z};
}
constexpr glm::quat toQuat(aiQuaternion const &aiquaternion)
{
    return glm::quat{aiquaternion.w, aiquaternion.x, aiquaternion.y, aiquaternion.z};
}

static float u8ToFloat(unsigned char v) { return v / 255.0f; }
static glm::vec3 getColor(aiMaterial const *material, glm::vec3 defaultColor, const char* key, unsigned int type, unsigned int idx)
{
    aiColor3D color;
    if(material->Get(key, type, idx, color) == AI_SUCCESS) {
        return {color.r, color.g, color.b};
    }

    return defaultColor;
}
static glm::vec4 getColor(aiMaterial const *material, glm::vec4 defaultColor, const char* key, unsigned int type, unsigned int idx)
{
    aiColor4D color;
    if(material->Get(key, type, idx, color) == AI_SUCCESS) {
        return {color.r, color.g, color.b, color.a};
    }

    return defaultColor;
}
static float getColor(aiMaterial const *material, float defaultColor, const char* key, unsigned int type, unsigned int idx)
{
    float color;
    if(material->Get(key, type, idx, color) == AI_SUCCESS) {
        return color;
    }

    return defaultColor;
}
static void setMissingTextures(engine::Material::Textures &material, engine::Material::Textures const &defaultMaterial)
{
    if(!material.albedo      ) material.albedo       = defaultMaterial.albedo;
    if(!material.metallic    ) material.metallic     = defaultMaterial.metallic;
    if(!material.roughness       ) material.roughness        = defaultMaterial.roughness;
    if(!material.ambient     ) material.ambient      = defaultMaterial.ambient;
    if(!material.normal      ) material.normal       = defaultMaterial.normal;
    if(!material.displacement) material.displacement = defaultMaterial.displacement;
    if(!material.alpha       ) material.alpha        = defaultMaterial.alpha;
}

aiNodeAnim const *findNodeAnim(aiAnimation const *animation, std::string_view nodeName)
{
    for(unsigned i = 0; i < animation->mNumChannels; ++i) 
    {
        aiNodeAnim const *node = animation->mChannels[i];
        if(std::string_view{node->mNodeName.C_Str()} == nodeName) 
            return node;
    }

    return nullptr;
}

static engine::Material getDefaultMaterial(ecs::registry &registry)
{
    ecs::entity white = 0;
    ecs::entity blue = 0;
    ecs::entity black = 0;
    ecs::entity tile = 0;

    for(ecs::entity e_texture : registry.view<engine::Texture>())
    {
        auto &texture = registry.get<engine::Texture>(e_texture);

        if(texture.path == "default/white")
            white = e_texture;
        if(texture.path == "default/blue")
            blue = e_texture;
        if(texture.path == "default/black")
            black = e_texture;
        if(texture.path == "default/tile")
            tile = e_texture;
    }

    if(!white)
        white = registry.create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                1, 1, 1
            }.data()},
            // .grayscale = true,
            .path = "default/white"
        });
    if(!blue)
        blue = registry.create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                0, 0, 1
            }.data()},
            // .grayscale = true,
            .path = "default/blue"
        });
    if(!black)
        black = registry.create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                0, 0, 0
            }.data()},
            // .grayscale = true,
            .path = "default/black"
        });
    if(!tile)
        tile = registry.create(engine::Texture{
            .data = engine::Bitmap<float>{8, 8, 3, std::array<float, 8*8*3>{
                1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 
                0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 
                1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 
                0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 
                1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 
                0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 
                1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 
                0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 1.0, 1.0, 1.0, 
            }.data()},
            // .grayscale = true,
            .path = "default/tile"
        });

    return {
        .textures = {
            .albedo = tile,
            .metallic = black,
            .roughness = white,
            .ambient = white,
            .normal = blue,
            .displacement = black,
            .alpha = white 
        },
        .properties = {
            .ambient       = {0.1f, 0.1f, 0.1f},
            .albedo        = {0.8f, 0.8f, 0.8f, 1.0f},
            .specular      = {0.5f, 0.5f, 0.5f},
            .emission      = {0.0f, 0.0f, 0.0f},
    
            .shininess = 32.0f,
            .ior       = 1.5f
        }
    };
}
static ecs::entity fromRawAssimpTexture(ecs::registry &registry, aiTexture const *texture)
{
    ENGINE_ASSERT(texture->mHeight == 0);
    unsigned const width = static_cast<unsigned>(texture->mWidth);
    unsigned const height = static_cast<unsigned>(texture->mHeight);

    if(!texture->pcData)
    {
        ENGINE_CORE_ERROR("aiTexture has no texel data");
        return 0;
    }

    engine::Texture result;
    result.data = engine::Bitmap<float>{width, height, 4};

    for (size_t i = 0; i < width * height; ++i) 
    {
        aiTexel const &texel = texture->pcData[i];
        result.data.setPixel(static_cast<unsigned>(i % width), static_cast<unsigned>(i / width), {
            u8ToFloat(texel.r),
            u8ToFloat(texel.g),
            u8ToFloat(texel.b),
            u8ToFloat(texel.a)
        });
    }


    return registry.create(std::move(result));
}
void engine::loader::ModelLoader::loadMaterialTexture(aiMaterial const *material, aiTextureType const type, ecs::entity &out)
{
    static engine::loader::TextureLoader loader;

    if(material->GetTextureCount(type) == 0 || out != 0)
        return;

    // load the first texture (multiple texture of the same type are not supported)
    std::string directory = std::filesystem::path{currentModel->path}.parent_path().string();
    aiString str;
    if(material->GetTexture(type, 0, &str) != AI_SUCCESS) 
        return; 
    bool srgb = type == aiTextureType_DIFFUSE;

    aiTexture const *embedded = currentScene->GetEmbeddedTexture(str.C_Str());
    if(embedded)
    {
        if(embedded->mHeight == 0)
        {
            MODEL_LOADER_TRACE("Loading embedded compressed texture \"{}\"", embedded->mFilename.C_Str());
            out = loader.load(*currentRegistry, embedded->mWidth, embedded->pcData, LoadingFlags::NONE);
        } else
        {
            MODEL_LOADER_TRACE("Loading embedded raw texture \"{}\"", embedded->mFilename.C_Str());
            out = fromRawAssimpTexture(*currentRegistry, embedded);
        }

        if(out)
        {
            currentRegistry->get<engine::Texture>(out).path = embedded->mFilename.C_Str();
            currentRegistry->get<engine::Texture>(out).srgb = srgb;
        }
        return;
    }

    std::string filepath = directory + '/' + str.C_Str();

    for(auto e_texture : currentRegistry->view<engine::Texture>()) {
        auto const &texture = currentRegistry->get<engine::Texture>(e_texture);
        if(texture.path == filepath) {
            out = e_texture;
            return;
        }
    }

    MODEL_LOADER_TRACE("Loading file texture \"{}\"", filepath);
    out = loader.load(*currentRegistry, filepath, LoadingFlags::NONE);

    if(out)
    {
        currentRegistry->get<engine::Texture>(out).srgb = srgb;
    }
}
engine::Material engine::loader::ModelLoader::convertMaterial(aiMaterial const *aimaterial, engine::Material::Properties const &defaultProperties)
{
    engine::Material material;

    // https://github.com/assimp/assimp/issues/430
    loadMaterialTexture(aimaterial, aiTextureType_DIFFUSE,           material.textures.albedo      );
    loadMaterialTexture(aimaterial, aiTextureType_NORMALS,           material.textures.normal      );
    loadMaterialTexture(aimaterial, aiTextureType_HEIGHT,            material.textures.normal      ); 
    loadMaterialTexture(aimaterial, aiTextureType_DISPLACEMENT,      material.textures.displacement);
    loadMaterialTexture(aimaterial, aiTextureType_AMBIENT_OCCLUSION, material.textures.ambient     );
    loadMaterialTexture(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, material.textures.roughness   );
    loadMaterialTexture(aimaterial, aiTextureType_TRANSMISSION,      material.textures.alpha       );
    loadMaterialTexture(aimaterial, aiTextureType_METALNESS,         material.textures.metallic    );

    material.properties = {
        .ambient       = getColor(aimaterial, defaultProperties.ambient,       AI_MATKEY_COLOR_AMBIENT),
        .albedo        = getColor(aimaterial, defaultProperties.albedo,       AI_MATKEY_COLOR_DIFFUSE),
        .specular      = getColor(aimaterial, defaultProperties.specular,      AI_MATKEY_COLOR_SPECULAR),
        .emission      = getColor(aimaterial, defaultProperties.emission,      AI_MATKEY_COLOR_EMISSIVE),

        .shininess     = getColor(aimaterial, defaultProperties.shininess,     AI_MATKEY_SHININESS),
        .metallic      = getColor(aimaterial, defaultProperties.metallic,      AI_MATKEY_METALLIC_FACTOR),
        .ior           = getColor(aimaterial, defaultProperties.ior,           AI_MATKEY_REFRACTI)
    };

    return material;
}

static void calculateMissingPrimitives(engine::Mesh &mesh)
{
    ENGINE_ASSERT(!mesh.geometry.positions.empty());

    bool indexed = !mesh.geometry.indices.empty();
    if(mesh.geometry.texCoords.empty())
    {
        MODEL_LOADER_TRACE("Calculating missing texcoords");
        mesh.geometry.texCoords.resize(mesh.geometry.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.geometry.indices.size() : mesh.geometry.positions.size()); ++i)
        {
            unsigned index = indexed ? mesh.geometry.indices[i] : i;

            mesh.geometry.texCoords[index] = std::array<glm::vec2, 6>{
                glm::vec2{0, 0},
                glm::vec2{0, 1},
                glm::vec2{1, 1},
                glm::vec2{1, 0},
                glm::vec2{1, 1},
                glm::vec2{0, 0},
            }[i%6];
        }
    }

    if(mesh.geometry.normals.empty())
    {
        MODEL_LOADER_TRACE("Calculating missing normals");
        mesh.geometry.normals.resize(mesh.geometry.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.geometry.indices.size() : mesh.geometry.positions.size()); i+=3)
        {
            size_t i0 = indexed ? mesh.geometry.indices[i+0] : i+0;
            size_t i1 = indexed ? mesh.geometry.indices[i+1] : i+1;
            size_t i2 = indexed ? mesh.geometry.indices[i+2] : i+2;

            glm::vec3 e1 = mesh.geometry.positions[i1] - mesh.geometry.positions[i0];
            glm::vec3 e2 = mesh.geometry.positions[i2] - mesh.geometry.positions[i0];
            glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
            mesh.geometry.normals[i0] = { normal, 0 };
            mesh.geometry.normals[i1] = { normal, 0 };
            mesh.geometry.normals[i2] = { normal, 0 };
        }
    }

    if(mesh.geometry.tangents.empty())
    {
        MODEL_LOADER_TRACE("Calculating missing tangents");
        mesh.geometry.tangents.resize(mesh.geometry.positions.size());
        for(size_t i = 0; i < (indexed ? mesh.geometry.indices.size() : mesh.geometry.positions.size()); i+=3)
        {
            size_t i0 = indexed ? mesh.geometry.indices[i+0] : i+0;
            size_t i1 = indexed ? mesh.geometry.indices[i+1] : i+1;
            size_t i2 = indexed ? mesh.geometry.indices[i+2] : i+2;

            glm::vec3 edge1 = mesh.geometry.positions[i1] - mesh.geometry.positions[i0];
            glm::vec3 edge2 = mesh.geometry.positions[i2] - mesh.geometry.positions[i0];
            glm::vec2 deltaUV1 = mesh.geometry.texCoords[i1] - mesh.geometry.texCoords[i0];
            glm::vec2 deltaUV2 = mesh.geometry.texCoords[i2] - mesh.geometry.texCoords[i0]; 

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent = {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
            };
            glm::vec3 normal = mesh.geometry.normals[i0];
            tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));
            
            mesh.geometry.tangents[i0] = { tangent, 0 };
            mesh.geometry.tangents[i1] = { tangent, 0 };
            mesh.geometry.tangents[i2] = { tangent, 0 };
        }
    }
}
static void optimizeMesh(engine::Mesh &mesh)
{
    engine::Mesh oldMesh = mesh;
    bool indexed = !oldMesh.geometry.indices.empty();

    size_t index_count = indexed ? oldMesh.geometry.indices.size() : oldMesh.geometry.positions.size();
    size_t vertex_count = indexed ? oldMesh.geometry.positions.size() : index_count;
    std::vector<meshopt_Stream> streams = {
        meshopt_Stream{oldMesh.geometry.positions.data(), sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{oldMesh.geometry.texCoords.data(), sizeof(glm::vec2), sizeof(glm::vec2)},
        meshopt_Stream{oldMesh.geometry.normals  .data(), sizeof(glm::vec4), sizeof(glm::vec4)},
        meshopt_Stream{oldMesh.geometry.tangents .data(), sizeof(glm::vec4), sizeof(glm::vec4)} 
    };

    if(!oldMesh.geometry.boneIDs.empty())
    {
        streams.emplace_back(meshopt_Stream{oldMesh.geometry.boneIDs.data(), sizeof(glm::ivec4), sizeof(glm::ivec4)});
        streams.emplace_back(meshopt_Stream{oldMesh.geometry.weights.data(), sizeof(glm::vec4),  sizeof(glm::vec4)});
    }

    std::vector<unsigned int> remap(vertex_count);
    size_t new_vertex_count = meshopt_generateVertexRemapMulti(remap.data(), indexed ? oldMesh.geometry.indices.data() : nullptr, index_count, vertex_count, streams.data(), streams.size());
    mesh.geometry.indices.resize(index_count); meshopt_remapIndexBuffer(mesh.geometry.indices.data(), indexed ? oldMesh.geometry.indices.data() : nullptr, index_count, remap.data());
    mesh.geometry.positions.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.positions.data(), streams[0].data, vertex_count, streams[0].size, remap.data());
    mesh.geometry.texCoords.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.texCoords.data(), streams[1].data, vertex_count, streams[1].size, remap.data());
    mesh.geometry.normals  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.normals  .data(), streams[2].data, vertex_count, streams[2].size, remap.data());
    mesh.geometry.tangents .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.tangents .data(), streams[3].data, vertex_count, streams[3].size, remap.data());
    if(!oldMesh.geometry.boneIDs.empty())
    {
        mesh.geometry.boneIDs  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.boneIDs  .data(), streams[4].data, vertex_count, streams[4].size, remap.data());
        mesh.geometry.weights  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.geometry.weights  .data(), streams[5].data, vertex_count, streams[5].size, remap.data());
    }

    if(oldMesh.geometry.indices.size() == mesh.geometry.indices.size() && oldMesh.geometry.positions.size() == mesh.geometry.positions.size())
        MODEL_LOADER_TRACE("Optimized mesh. Nothing changed.");
    else
        MODEL_LOADER_TRACE("Optimized mesh. Had {} indices and {} vertices. Has {} indices and {} vertices", oldMesh.geometry.indices.size(), oldMesh.geometry.positions.size(), mesh.geometry.indices.size(), mesh.geometry.positions.size());
}
static void moveMesh(engine::Mesh::Geometry &primitives, glm::mat4 const &mat)
{
    if(mat == glm::mat4{1.0f})
        return;

    MODEL_LOADER_TRACE("Applying transformation to a mesh.");

    glm::mat4 normalMat = glm::inverse(glm::transpose(mat));

    for(auto &position : primitives.positions)
        position = mat * position;
    for(auto &normal : primitives.normals)
        normal = normalMat * normal;
    for(auto &tangent : primitives.tangents)
        tangent = normalMat * tangent;
}

static void extractVertexData(aiMesh const *aimesh, engine::Mesh &mesh)
{
    for(unsigned i = 0; i < aimesh->mNumVertices; ++i) {
        mesh.geometry.positions.emplace_back(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z, 1);
        if(aimesh->HasNormals())
            mesh.geometry.normals.emplace_back(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z, 0);
        if(aimesh->HasTangentsAndBitangents())
            mesh.geometry.tangents.emplace_back(aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z, 0);
        if(aimesh->HasTextureCoords(0))
            mesh.geometry.texCoords.emplace_back(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
    }
    for(unsigned i = 0; i < aimesh->mNumFaces; ++i) {
        aiFace face = aimesh->mFaces[i];
        for(unsigned j = 0; j < face.mNumIndices; ++j) {
            mesh.geometry.indices.push_back(face.mIndices[j]);
        }
    }
}
static void extractBoneData(aiMesh const *aimesh, engine::Mesh &mesh, engine::Skeleton &skeleton)
{
    // i hate it -- april 2025
    // it works -- october 2025
    glm::ivec4 boneIDs{-1}; mesh.geometry.boneIDs.resize(mesh.geometry.positions.size(), boneIDs); 
    glm::vec4 weights{0}; mesh.geometry.weights.resize(mesh.geometry.positions.size(), weights);
    for(unsigned boneIndex = 0; boneIndex < aimesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        aiBone const *bone = aimesh->mBones[boneIndex];
        std::string boneName = bone->mName.C_Str();
        if(skeleton.boneMap.find(boneName) == skeleton.boneMap.end()) 
        {
            unsigned id = skeleton.boneMap.size();
            skeleton.bindTransform.emplace_back(toMat4(bone->mOffsetMatrix));
            skeleton.boneMap.try_emplace(boneName, id);
            boneID = id;
        } else 
        {
            boneID = skeleton.boneMap.at(boneName);
        }
        ENGINE_ASSERT(boneID != -1);

        for(unsigned weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) 
        {
            unsigned vertexID = bone->mWeights[weightIndex].mVertexId;
            ENGINE_ASSERT(vertexID < mesh.geometry.positions.size());
            // record it in the first uninitialized slot
            for(unsigned i = 0; i < 4; ++i) 
            { 
                if(mesh.geometry.boneIDs[vertexID][i] == -1) 
                {
                    mesh.geometry.boneIDs[vertexID][i] = boneID;
                    mesh.geometry.weights[vertexID][i] = bone->mWeights[weightIndex].mWeight;
                    break;
                }
            }
        }
    }
}
void normalizeWeights(engine::Mesh::Geometry &geometry)
{
    for(auto &weight : geometry.weights)
        if(weight[0] + weight[1] + weight[2] + weight[3] > 1e-6f)
            weight /= weight[0] + weight[1] + weight[2] + weight[3];
}
engine::Mesh engine::loader::ModelLoader::processMesh(aiMesh const *aimesh, glm::mat4 const &transform)
{
    MODEL_LOADER_TRACE("Loading mesh \"{}\"", aimesh->mName.C_Str());
    engine::Mesh mesh;
    extractVertexData(aimesh, mesh);

    if(aimesh->HasBones())
    {
        extractBoneData(aimesh, mesh, currentModel->skeleton);
        normalizeWeights(mesh.geometry);
    }

    auto defaultMaterial = getDefaultMaterial(*currentRegistry);
    if(!currentScene->HasMaterials())
    {
        mesh.material = defaultMaterial;
    }
    else
    {
        aiMaterial const *material = currentScene->mMaterials[aimesh->mMaterialIndex];
        mesh.material = convertMaterial(material, defaultMaterial.properties);
        setMissingTextures(mesh.material.textures, defaultMaterial.textures);
    }

    calculateMissingPrimitives(mesh);
    optimizeMesh(mesh);

    // Apply transformation only for static meshes. 
    // Models with bones should use the bone transformations.
    if(!aimesh->HasBones())
        moveMesh(mesh.geometry, transform);

    return mesh;
}
void engine::loader::ModelLoader::processNodeMeshes(aiNode const *node, glm::mat4 parentTransform = glm::mat4{1.0f})
{
    MODEL_LOADER_TRACE("Processing node \"{}\"", node->mName.C_Str());
    glm::mat4 nodeTransform = parentTransform * toMat4(node->mTransformation);
    for(unsigned i = 0; i < node->mNumMeshes; ++i) {
        currentModel->meshes.emplace_back(std::move(processMesh(currentScene->mMeshes[node->mMeshes[i]], nodeTransform)));
    }
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        processNodeMeshes(node->mChildren[i], nodeTransform);
    }
}

static void processAnimationNode(engine::Animation &result, aiAnimation const *animation, engine::Skeleton const &skeleton, aiNode const *node)
{
    std::string nodeName = node->mName.C_Str();
    aiNodeAnim const *nodeAnim = findNodeAnim(animation, nodeName);

    if(nodeAnim && skeleton.boneMap.find(nodeName) != skeleton.boneMap.end()) {
        auto &keyframes = result.bones.at(skeleton.boneMap.at(nodeName));
        for(unsigned i = 0; i < nodeAnim->mNumPositionKeys; ++i)
        {
            auto const &key = nodeAnim->mPositionKeys[i];
            keyframes.positions.emplace_back(engine::Animation::PositionKey{
                .value = toVec3(key.mValue),
                .timeTicks = static_cast<float>(key.mTime)
            });
        }
        for(unsigned i = 0; i < nodeAnim->mNumRotationKeys; ++i)
        {
            auto const &key = nodeAnim->mRotationKeys[i];
            keyframes.orientations.emplace_back(engine::Animation::OrientationKey{
                .value = glm::normalize(toQuat(key.mValue)),
                .timeTicks = static_cast<float>(key.mTime)
            });
        }
        for(unsigned i = 0; i < nodeAnim->mNumScalingKeys; ++i)
        {
            auto const &key = nodeAnim->mScalingKeys[i];
            keyframes.scales.emplace_back(engine::Animation::ScaleKey{
                .value = toVec3(key.mValue),
                .timeTicks = static_cast<float>(key.mTime)
            });
        }
    }

    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        processAnimationNode(result, animation, skeleton, node->mChildren[i]);
    }
}
engine::Animation engine::loader::ModelLoader::processAnimation(aiAnimation const *animation)
{
    MODEL_LOADER_TRACE("Processing animation \"{}\"", animation->mName.C_Str());

    engine::Animation result;
    result.durationTicks = (float) animation->mDuration;
    result.ticksPerSecond = (animation->mTicksPerSecond > 0) ? (float) animation->mTicksPerSecond : 24.0f;
    result.name = animation->mName.C_Str();
    result.bones.resize(currentModel->skeleton.boneMap.size());

    processAnimationNode(result, animation, currentModel->skeleton, currentScene->mRootNode);

    for(auto &bone : result.bones)
    {
        std::sort(bone.positions   .begin(), bone.positions   .end(), [](engine::Animation::PositionKey    const &first, engine::Animation::PositionKey    const &second){ return first.timeTicks < second.timeTicks; });
        std::sort(bone.orientations.begin(), bone.orientations.end(), [](engine::Animation::OrientationKey const &first, engine::Animation::OrientationKey const &second){ return first.timeTicks < second.timeTicks; });
        std::sort(bone.scales      .begin(), bone.scales      .end(), [](engine::Animation::ScaleKey       const &first, engine::Animation::ScaleKey       const &second){ return first.timeTicks < second.timeTicks; });
    }

    return result;
}

static void calculateParent(engine::Skeleton &skeleton, aiNode const *node, int parent)
{
    std::string nodeName = node->mName.C_Str();

    if(skeleton.boneMap.find(nodeName) != skeleton.boneMap.end())
    {
        unsigned bone = skeleton.boneMap.at(nodeName);
        skeleton.parents.at(bone) = parent;
        skeleton.nodeTransform.at(bone) = toMat4(node->mTransformation);
        parent = bone;
    }

    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        calculateParent(skeleton, node->mChildren[i], parent);
    }
}

ecs::entity engine::loader::ModelLoader::load()
{
    if(currentScene->HasMeshes())
        MODEL_LOADER_TRACE("Loading {} meshes.", currentScene->mNumMeshes);
    if(currentScene->HasAnimations())
        MODEL_LOADER_TRACE("Loading {} animations.", currentScene->mNumAnimations);

    processNodeMeshes(currentScene->mRootNode);

    MODEL_LOADER_TRACE("Model has {} bones.", currentModel->skeleton.boneMap.size());

    currentModel->skeleton.parents.resize(currentModel->skeleton.boneMap.size());
    currentModel->skeleton.nodeTransform.resize(currentModel->skeleton.boneMap.size());
    calculateParent(currentModel->skeleton, currentScene->mRootNode, -1);

    for(unsigned i = 0; i < currentScene->mNumAnimations; ++i)
    {
        currentModel->animations.emplace_back(std::move(processAnimation(currentScene->mAnimations[i])));
    }

    // TODO: morph targets

    return currentRegistry->create(std::move(*currentModel));
}

ecs::entity engine::loader::ModelLoader::load(ecs::registry &reg, std::string_view path, LoadingFlags flags)
{
    MODEL_LOADER_TRACE("---");
    MODEL_LOADER_TRACE("Loading model \"{}\"", path);
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    aiScene const *scene = importer.ReadFile(std::string{path}, 
        aiProcess_SplitLargeMeshes      |
        aiProcess_GenNormals            |
        aiProcess_GenUVCoords           |
        aiProcess_FindInvalidData       |
        aiProcess_CalcTangentSpace      |
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType           |
        aiProcess_OptimizeGraph         |
        aiProcess_OptimizeMeshes        |
        aiProcess_ValidateDataStructure |
        aiProcess_LimitBoneWeights      |
        (bool(flags & LoadingFlags::MODEL_FLIP_WINDING_ORDER) ? aiProcess_FlipWindingOrder : 0) |
        (bool(flags & LoadingFlags::MODEL_FLIP_TEXTURES)      ? aiProcess_FlipUVs : 0)
    );

    if(!scene)
    {
        ENGINE_CORE_ERROR("Error parsing \"{}\": {}", path, importer.GetErrorString());
        return 0;
    }

    Model model;
    model.path = path;

    currentModel = &model;
    currentScene = scene;
    currentRegistry = &reg;
    currentFlags = flags;
    currentModel->skeleton.globalInverseTransform = glm::inverse(toMat4(currentScene->mRootNode->mTransformation));


    return load();
}
ecs::entity engine::loader::ModelLoader::load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags)
{
    MODEL_LOADER_TRACE("---");
    MODEL_LOADER_TRACE("Loading model from memory");
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    aiScene const *scene = importer.ReadFileFromMemory(data, size, 
        aiProcess_SplitLargeMeshes      |
        aiProcess_GenNormals            |
        aiProcess_GenUVCoords           |
        aiProcess_FindInvalidData       |
        aiProcess_CalcTangentSpace      |
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType           |
        aiProcess_OptimizeGraph         |
        aiProcess_OptimizeMeshes        |
        aiProcess_ValidateDataStructure |
        aiProcess_LimitBoneWeights      |
        (bool(flags & LoadingFlags::MODEL_FLIP_WINDING_ORDER) ? aiProcess_FlipWindingOrder : 0) |
        (bool(flags & LoadingFlags::MODEL_FLIP_TEXTURES)      ? aiProcess_FlipUVs : 0)
    );

    if(!scene)
    {
        ENGINE_CORE_ERROR("Error parsing from memory: {}", importer.GetErrorString());
        return 0;
    }

    Model model;
    model.path = "memory";

    currentModel = &model;
    currentScene = scene;
    currentRegistry = &reg;
    currentFlags = flags;
    currentModel->skeleton.globalInverseTransform = glm::inverse(toMat4(currentScene->mRootNode->mTransformation));


    return load();
}
