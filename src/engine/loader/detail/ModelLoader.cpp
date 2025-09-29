#include "Loaders.hpp"
#include "meshoptimizer.h"

#include <filesystem>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

// For easier access (previous attempt turned into 12-argument mess)
static aiScene const *currentScene;
static engine::Model *currentModel;
static ecs::registry *currentRegistry;

static void calculateMissingPrimitives(engine::Mesh &mesh)
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
static void optimizeMesh(engine::Mesh &mesh)
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

    if(!oldMesh.skeleton.boneIDs.empty())
    {
        streams.emplace_back(meshopt_Stream{oldMesh.skeleton.boneIDs  .data(), sizeof(glm::ivec4), sizeof(glm::ivec4)});
        streams.emplace_back(meshopt_Stream{oldMesh.skeleton.weights  .data(), sizeof(glm::vec4),  sizeof(glm::vec4)});
    }

    std::vector<unsigned int> remap(vertex_count);
    size_t new_vertex_count = meshopt_generateVertexRemapMulti(remap.data(), indexed ? oldMesh.indices.data() : nullptr, index_count, vertex_count, streams.data(), streams.size());
    mesh.indices.resize(index_count); meshopt_remapIndexBuffer(mesh.indices.data(), indexed ? oldMesh.indices.data() : nullptr, index_count, remap.data());
    mesh.positions.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.positions.data(), streams[0].data, vertex_count, streams[0].size, remap.data());
    mesh.texCoords.resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.texCoords.data(), streams[1].data, vertex_count, streams[1].size, remap.data());
    mesh.normals  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.normals  .data(), streams[2].data, vertex_count, streams[2].size, remap.data());
    mesh.tangents .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.tangents .data(), streams[3].data, vertex_count, streams[3].size, remap.data());
    if(!oldMesh.skeleton.boneIDs.empty())
    {
        mesh.skeleton.boneIDs  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.skeleton.boneIDs  .data(), streams[4].data, vertex_count, streams[4].size, remap.data());
        mesh.skeleton.weights  .resize(new_vertex_count); meshopt_remapVertexBuffer(mesh.skeleton.weights  .data(), streams[5].data, vertex_count, streams[5].size, remap.data());
    }
}

constexpr static glm::mat4 toMat4(aiMatrix4x4 const &from)
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
constexpr static glm::vec3 toVec3(aiVector3X const &aivector)
{
    return glm::vec3{(float) aivector.x, (float) aivector.y, (float) aivector.z};
}
constexpr static glm::quat toQuat(aiQuaternion const &aiquaternion)
{
    return glm::quat{aiquaternion.w, aiquaternion.x, aiquaternion.y, aiquaternion.z};
}

static engine::Material getDefaultMaterial()
{
    ecs::entity white = 0;
    ecs::entity blue = 0;
    ecs::entity black = 0;
    ecs::entity tile = 0;

    for(ecs::entity e_texture : currentRegistry->view<engine::Texture>())
    {
        auto &texture = currentRegistry->get<engine::Texture>(e_texture);

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
        white = currentRegistry->create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                1, 1, 1
            }.data()},
            .grayscale = true,
            .path = "default/white"
        });
    if(!blue)
        blue = currentRegistry->create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                0, 0, 1
            }.data()},
            .grayscale = true,
            .path = "default/blue"
        });
    if(!black)
        black = currentRegistry->create(engine::Texture{
            .data = engine::Bitmap<float>{1, 1, 3, std::array<float, 1*3>{
                0, 0, 0
            }.data()},
            .grayscale = true,
            .path = "default/black"
        });
    if(!tile)
        tile = currentRegistry->create(engine::Texture{
            .data = engine::Bitmap<float>{2, 2, 3, std::array<float, 4*3>{
                1, 1, 1, 0.5, 0.5, 0.5,
                0.5, 0.5, 0.5, 1, 1, 1 
            }.data()},
            .grayscale = true,
            .path = "default/tile"
        });

    return {
        .textures = {
            .ambient      = white,
            .albedo       = tile,
            .specular     = white,
            .normal       = blue,
            .displacement = black,
            .alpha        = white,
            .reflection   = black,
            .metallic     = black 
        },
        .properties = {
            .ambient       = {0.1f, 0.1f, 0.1f},
            .albedo       = {0.8f, 0.8f, 0.8f},
            .specular      = {0.5f, 0.5f, 0.5f},
            .transmittance = {0.0f, 0.0f, 0.0f},
            .emission      = {0.0f, 0.0f, 0.0f},
    
            .shininess = 32.0f,
            .ior       = 1.5f
        }
    };
}
static void loadMaterialTexture(aiMaterial const *material, aiTextureType const type, ecs::entity &out)
{
    static engine::detail::TextureLoader loader;

    if(material->GetTextureCount(type) == 0 || out != 0)
        return;

    // load the first texture (multiple texture of the same type are not supported)
    std::string directory = std::filesystem::path{currentModel->path}.parent_path().string();
    aiString str;
    material->GetTexture(type, 0, &str); 
    std::string filepath = directory + '/' + str.C_Str();

    for(auto e_texture : currentRegistry->view<engine::Texture>()) {
        auto const &texture = currentRegistry->get<engine::Texture>(e_texture);
        if(texture.path == filepath) {
            out = e_texture;
            return;
        }
    }

    out = loader.load(*currentRegistry, filepath);
}
static glm::vec3 getColor(aiMaterial const *material, glm::vec3 defaultColor, const char* key, unsigned int type, unsigned int idx)
{
    aiColor3D color;
    if(material->Get(key, type, idx, color) == AI_SUCCESS) {
        return {color.r, color.g, color.b};
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
static engine::Material convertMaterial(aiMaterial const *aimaterial, engine::Material::Properties const &defaultProperties)
{
    engine::Material material;

    // https://github.com/assimp/assimp/issues/430
    loadMaterialTexture(aimaterial, aiTextureType_DIFFUSE,           material.textures.albedo      );
    loadMaterialTexture(aimaterial, aiTextureType_SPECULAR,          material.textures.specular    );
    loadMaterialTexture(aimaterial, aiTextureType_NORMALS,           material.textures.normal      );
    loadMaterialTexture(aimaterial, aiTextureType_HEIGHT,            material.textures.normal      ); 
    loadMaterialTexture(aimaterial, aiTextureType_DISPLACEMENT,      material.textures.displacement);
    loadMaterialTexture(aimaterial, aiTextureType_AMBIENT_OCCLUSION, material.textures.ambient     );
    loadMaterialTexture(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, material.textures.rough       );
    loadMaterialTexture(aimaterial, aiTextureType_TRANSMISSION,      material.textures.alpha       );
    loadMaterialTexture(aimaterial, aiTextureType_METALNESS,         material.textures.metallic    );

    material.properties = {
        .ambient       = getColor(aimaterial, defaultProperties.ambient,       AI_MATKEY_COLOR_AMBIENT),
        .albedo        = getColor(aimaterial, defaultProperties.albedo,       AI_MATKEY_COLOR_DIFFUSE),
        .specular      = getColor(aimaterial, defaultProperties.specular,      AI_MATKEY_COLOR_SPECULAR),
        .transmittance = getColor(aimaterial, defaultProperties.transmittance, AI_MATKEY_COLOR_TRANSPARENT),
        .emission      = getColor(aimaterial, defaultProperties.emission,      AI_MATKEY_COLOR_EMISSIVE),

        .shininess     = getColor(aimaterial, defaultProperties.shininess,     AI_MATKEY_SHININESS),
        .metallic      = getColor(aimaterial, defaultProperties.metallic,      AI_MATKEY_METALLIC_FACTOR),
        .ior           = getColor(aimaterial, defaultProperties.shininess,     AI_MATKEY_REFRACTI)
    };

    return material;
}
static void setMissingTextures(engine::Material::Textures &material, engine::Material::Textures const &defaultMaterial)
{
    if(!material.ambient)      material.ambient      = defaultMaterial.ambient;
    if(!material.albedo)      material.albedo      = defaultMaterial.albedo;
    if(!material.specular)     material.specular     = defaultMaterial.specular;
    if(!material.normal)       material.normal       = defaultMaterial.normal;
    if(!material.displacement) material.displacement = defaultMaterial.displacement;
    if(!material.alpha)        material.alpha        = defaultMaterial.alpha;
    if(!material.reflection)   material.reflection   = defaultMaterial.reflection;
}

static void extractVertexData(aiMesh const *aimesh, engine::Mesh &mesh)
{
    for(unsigned i = 0; i < aimesh->mNumVertices; ++i) {
        mesh.positions.emplace_back(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z, 1);
        if(aimesh->HasNormals())
            mesh.normals.emplace_back(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z, 0);
        if(aimesh->HasTangentsAndBitangents())
            mesh.tangents.emplace_back(aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z, 0);
        if(aimesh->HasTextureCoords(0))
            mesh.texCoords.emplace_back(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
    }
    for(unsigned i = 0; i < aimesh->mNumFaces; ++i) {
        aiFace face = aimesh->mFaces[i];
        for(unsigned j = 0; j < face.mNumIndices; ++j) {
            mesh.indices.push_back(face.mIndices[j]);
        }
    }
}
static void extractBoneData(aiMesh const *aimesh, engine::Mesh &mesh)
{
    // i hate it -- april 2025
    // it works -- october 2025
    static unsigned boneCounter = 0;
    glm::ivec4 boneIDs{-1}; mesh.skeleton.boneIDs.resize(mesh.positions.size(), boneIDs); 
    glm::vec4 weights{-1}; mesh.skeleton.weights.resize(mesh.positions.size(), weights);
    for(unsigned boneIndex = 0; boneIndex < aimesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        aiBone const *bone = aimesh->mBones[boneIndex];
        std::string boneName = bone->mName.C_Str();
        if(mesh.skeleton.boneMap.find(boneName) == mesh.skeleton.boneMap.end()) 
        {
            unsigned id = boneCounter;
            mesh.skeleton.tposeTransform.emplace_back(toMat4(bone->mOffsetMatrix));
            mesh.skeleton.boneMap.try_emplace(boneName, id);
            boneID = id;
            ++boneCounter;
        } else 
        {
            boneID = mesh.skeleton.boneMap.at(boneName);
        }
        ENGINE_ASSERT(boneID != -1);

        for(unsigned weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) 
        {
            unsigned vertexID = bone->mWeights[weightIndex].mVertexId;
            ENGINE_ASSERT(vertexID < mesh.positions.size());
            // record it in the first uninitialized slot
            for(unsigned i = 0; i < 4; ++i) 
            { 
                if(mesh.skeleton.boneIDs[vertexID][i] == -1) 
                {
                    mesh.skeleton.boneIDs[vertexID][i] = boneID;
                    mesh.skeleton.weights[vertexID][i] = bone->mWeights[weightIndex].mWeight;
                    break;
                }
            }
        }
    }
}
static engine::Mesh processMesh(aiMesh const *aimesh)
{
    engine::Mesh mesh;
    extractVertexData(aimesh, mesh);

    if(aimesh->HasBones())
    {
        extractBoneData(aimesh, mesh);
    }

    auto defaultMaterial = getDefaultMaterial();
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

    return mesh;
}
static void processNode(aiNode const *node)
{
    for(unsigned i = 0; i < node->mNumMeshes; ++i) {
        currentModel->meshes.emplace_back(std::move(processMesh(currentScene->mMeshes[node->mMeshes[i]])));
    }
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i]);
    }
}

ecs::entity engine::detail::ModelLoader::load(ecs::registry &reg, std::string_view path)
{
    ENGINE_CORE_TRACE("Loading model \"{}\"", path);
    Assimp::Importer importer;
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
        aiProcess_OptimizeMeshes
    );

    if(!scene)
    {
        ENGINE_CORE_ERROR("Error parsing \"{}\": {}", path, importer.GetErrorString());
        return 0;
    }

    Model model;

    currentModel = &model;
    currentScene = scene;
    currentRegistry = &reg;

    model.path = path;
    processNode(scene->mRootNode);

    for(auto &mesh : model.meshes)
    {
        calculateMissingPrimitives(mesh);
        optimizeMesh(mesh);
    }

    return currentRegistry->create(std::move(model));
}