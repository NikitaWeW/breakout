#include "Model.hpp"
#include "Model.hpp"
#include "assimp/postprocess.h"
#include <iostream>

constexpr inline glm::mat4 toMat4(aiMatrix4x4 const &from)
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
constexpr inline glm::vec3 toVec3(aiVector3X const &aivector)
{
    return glm::vec3{(float) aivector.x, (float) aivector.y, (float) aivector.z};
}
constexpr inline glm::quat toQuat(aiQuaternion const &aiquaternion)
{
    return glm::quat{aiquaternion.w, aiquaternion.x, aiquaternion.y, aiquaternion.z};
}

aiNodeAnim const *findNodeAnim(aiAnimation const *animation, std::string const &nodeName)
{
    for(unsigned i = 0; i < animation->mNumChannels; ++i) {
        aiNodeAnim const *node = animation->mChannels[i];
        if(std::string{node->mNodeName.C_Str()} == nodeName) return node;
    }

    return nullptr;
}

// wet, too lazy to complicate this with pointers to members
unsigned findPosition(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumPositionKeys > 0);
    for(unsigned i = 0; i < nodeAnim->mNumPositionKeys - 1; ++i) {
        float time = (float) nodeAnim->mPositionKeys[i + 1].mTime;
        if(animationTimeTicks < time) {
            return i;
        }
    }
    return 0;
}
unsigned findRotation(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumRotationKeys > 0);
    for(unsigned i = 0; i < nodeAnim->mNumRotationKeys - 1; ++i) {
        float time = (float) nodeAnim->mRotationKeys[i + 1].mTime;
        if(animationTimeTicks < time) {
            return i;
        }
    }
    return 0;
}
unsigned findScaling(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumScalingKeys > 0);
    for(unsigned i = 0; i < nodeAnim->mNumScalingKeys - 1; ++i) {
        float time = (float) nodeAnim->mScalingKeys[i + 1].mTime;
        if(animationTimeTicks < time) {
            return i;
        }
    }
    return 0;
}

glm::vec3 calculateInterpolatedPosition(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumPositionKeys > 0);
    if(nodeAnim->mNumPositionKeys == 1) {
        return toVec3(nodeAnim->mPositionKeys[0].mValue);
    }

    unsigned positionIndex = findPosition(animationTimeTicks, nodeAnim);
    unsigned nextPositionIndex = positionIndex + 1;
    if(nextPositionIndex >= nodeAnim->mNumPositionKeys) {
        return toVec3(nodeAnim->mPositionKeys[0].mValue);
    }

    float time1 = (float) nodeAnim->mPositionKeys[positionIndex].mTime;
    float time2 = (float) nodeAnim->mPositionKeys[nextPositionIndex].mTime;
    float deltatime = time2 - time1;
    float factor = (animationTimeTicks - time1) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);

    return glm::mix( // the stupid function is called mix, not lerp
        toVec3(nodeAnim->mPositionKeys[positionIndex].mValue),
        toVec3(nodeAnim->mPositionKeys[nextPositionIndex].mValue),
        factor
    );
}
glm::quat calculateInterpolatedRotation(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumRotationKeys > 0);
    if(nodeAnim->mNumRotationKeys == 1) {
        return toQuat(nodeAnim->mRotationKeys[0].mValue);
    }

    unsigned rotationIndex = findRotation(animationTimeTicks, nodeAnim);
    unsigned nextRotationIndex = rotationIndex + 1;
    if(nextRotationIndex >= nodeAnim->mNumRotationKeys) {
        return toQuat(nodeAnim->mRotationKeys[0].mValue);
    }

    float time1 = (float) nodeAnim->mRotationKeys[rotationIndex].mTime;
    float time2 = (float) nodeAnim->mRotationKeys[nextRotationIndex].mTime;
    float deltatime = time2 - time1;
    float factor = (animationTimeTicks - time1) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);

    return glm::normalize(glm::slerp(
        toQuat(nodeAnim->mRotationKeys[rotationIndex].mValue),
        toQuat(nodeAnim->mRotationKeys[nextRotationIndex].mValue),
        factor
    ));
}
glm::vec3 calculateInterpolatedScaling(float animationTimeTicks, aiNodeAnim const *nodeAnim)
{
    assert(nodeAnim->mNumScalingKeys > 0);
    if(nodeAnim->mNumScalingKeys == 1) {
        return toVec3(nodeAnim->mScalingKeys[0].mValue);
    }

    unsigned scalingIndex = findScaling(animationTimeTicks, nodeAnim);
    unsigned nextScalingIndex = scalingIndex + 1;
    if(nextScalingIndex >= nodeAnim->mNumScalingKeys) {
        return toVec3(nodeAnim->mScalingKeys[0].mValue);
    }

    float time1 = (float) nodeAnim->mScalingKeys[scalingIndex].mTime;
    float time2 = (float) nodeAnim->mScalingKeys[nextScalingIndex].mTime;
    float deltatime = time2 - time1;
    float factor = (animationTimeTicks - time1) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);

    return glm::mix(
        toVec3(nodeAnim->mScalingKeys[scalingIndex].mValue),
        toVec3(nodeAnim->mScalingKeys[nextScalingIndex].mValue),
        factor
    );
}

void processAnimationNode( // ignore the argument count
    aiNode const *sceneNode, 
    aiAnimation const *animation, float animationTimeTicks,  
    aiAnimation const *secondAnimation, float secondAnimationTimeTicks, float factor,
    glm::mat4 const &parentTransformation, glm::mat4 const &globalInverseTransform, 
    std::map<std::string, unsigned> const &boneMap, 
    std::vector<glm::mat4> &boneTransformations, std::vector<glm::mat4> const &tposeTransform
)
{
    std::string nodeName = sceneNode->mName.C_Str();
    glm::mat4 nodeTransformation = toMat4(sceneNode->mTransformation);
    aiNodeAnim const *nodeAnim = findNodeAnim(animation, nodeName);
    
    if(nodeAnim) {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        
        aiNodeAnim const *secondNodeAnim = secondAnimation ? findNodeAnim(secondAnimation, nodeName) : nullptr;
        if(secondNodeAnim) {
            position = glm::mix  (calculateInterpolatedPosition(animationTimeTicks, nodeAnim), calculateInterpolatedPosition(secondAnimationTimeTicks, secondNodeAnim), factor);
            rotation = glm::slerp(calculateInterpolatedRotation(animationTimeTicks, nodeAnim), calculateInterpolatedRotation(secondAnimationTimeTicks, secondNodeAnim), factor);
            scale    = glm::mix  (calculateInterpolatedScaling (animationTimeTicks, nodeAnim), calculateInterpolatedScaling (secondAnimationTimeTicks, secondNodeAnim), factor);
        } else {
            position = calculateInterpolatedPosition(animationTimeTicks, nodeAnim);
            rotation = calculateInterpolatedRotation(animationTimeTicks, nodeAnim);
            scale    = calculateInterpolatedScaling (animationTimeTicks, nodeAnim);
        }

        nodeTransformation = 
            glm::translate(glm::mat4{1.0f}, position) * 
            glm::mat4_cast(rotation) * 
            glm::scale(glm::mat4{1.0f}, scale);
    }
    glm::mat4 globalTransformation = parentTransformation * nodeTransformation;

    if(boneMap.find(nodeName) != boneMap.end()) {
        glm::mat4 &modelToBoneSpace = boneTransformations.at(boneMap.at(nodeName));
        modelToBoneSpace = globalInverseTransform * globalTransformation * tposeTransform.at(boneMap.at(nodeName));
    }

    for(unsigned i = 0; i < sceneNode->mNumChildren; ++i) {
        processAnimationNode(sceneNode->mChildren[i], animation, animationTimeTicks, secondAnimation, secondAnimationTimeTicks, factor, globalTransformation, globalInverseTransform, boneMap, boneTransformations, tposeTransform);
    }
}

void model::Model::processNode(aiNode const *node, int flags, aiScene const *scene)
{
    for(unsigned i = 0; i < node->mNumMeshes; ++i) {
        m_meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], flags, scene));
    }
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], flags, scene);
    }
}

void makeDrawable(game::Drawable &drawable, model::MeshData &data) 
{
    drawable.vb = opengl::VertexBuffer{
        data.positions.size()     * sizeof(data.positions[0]) + 
        data.normals.size()       * sizeof(data.normals[0]) + 
        data.textureCoords.size() * sizeof(data.textureCoords[0]) + 
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
        data.textureCoords.size() * sizeof(data.textureCoords[0]), 
        data.textureCoords.data());
    glBufferSubData(GL_ARRAY_BUFFER, 
        data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0]), 
        data.tangents.size() * sizeof(data.tangents[0]), 
        data.tangents.data());
    if(data.boneIDs.size() != 0) {
        glBufferSubData(GL_ARRAY_BUFFER, 
            data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]), 
            data.boneIDs.size() * sizeof(data.boneIDs[0]), 
            data.boneIDs.data());
        glBufferSubData(GL_ARRAY_BUFFER, 
            data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]) + data.boneIDs.size() * sizeof(data.boneIDs[0]), 
            data.weights.size() * sizeof(data.weights[0]), 
            data.weights.data());
    }
    /*
        Index|Name
        -----|----------
          0  | positions
          1  | normals
          2  | texCoords
          3  | tangents
          4  | boneIDs
          5  | weights
    */

    drawable.ib = opengl::IndexBuffer{data.indices.size() * sizeof(data.indices[0]), data.indices.data()};
    drawable.va = opengl::VertexArray{drawable.vb, opengl::VertexBufferLayout{
        {4, GL_FLOAT, 0},
        {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0])},
        {2, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0])},
        {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0])},
        {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0]) + data.tangents.size() * sizeof(data.tangents[0])},
        {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0]) + data.tangents.size() * sizeof(data.tangents[0]) + data.boneIDs.size() * sizeof(data.boneIDs[0])},
    }};
    drawable.count = data.indices.size();
}
void extractBones(model::MeshData &data, std::map<std::string, unsigned> &boneMap, std::vector<glm::mat4> &boneTransformations, aiMesh const *aimesh, aiScene const *scene) 
{
    static unsigned boneCounter = 0;
    std::array<int, model::MAX_BONES_PER_VERTEX> boneIDs{}; boneIDs.fill(-1); data.boneIDs.resize(data.positions.size(), boneIDs); // i hate it
    std::array<float, model::MAX_BONES_PER_VERTEX> weights{}; weights.fill(-1); data.weights.resize(data.positions.size(), weights);
    for(unsigned boneIndex = 0; boneIndex < aimesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        aiBone *bone = aimesh->mBones[boneIndex];
        std::string boneName = bone->mName.C_Str();
        if(boneMap.find(boneName) == boneMap.end()) {
            unsigned id = boneCounter++;
            boneTransformations.push_back(toMat4(bone->mOffsetMatrix));
            assert(boneTransformations.size() - 1 == id);
            boneMap.insert({boneName, id});
            boneID = id;
        } else {
            boneID = boneMap.at(boneName);
        }
        assert(boneID != -1);

        for(unsigned weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            unsigned vertexID = bone->mWeights[weightIndex].mVertexId;
            assert(vertexID < data.positions.size());
            for(unsigned i = 0; i < model::MAX_BONES_PER_VERTEX; ++i) { // record it in the first uninitialised slot
                if(data.boneIDs[vertexID][i] == -1) {
                    data.boneIDs[vertexID][i] = boneID;
                    data.weights[vertexID][i] = bone->mWeights[weightIndex].mWeight;
                    break;
                }
            }
        }
    }
}
void extractVertexData(model::MeshData &data, aiMesh const *aimesh, aiScene const *scene)
{
    for(unsigned i = 0; i < aimesh->mNumVertices; ++i) {
        // i use vec4's for potential byte alignment. lets hope it wont be that bad on large models
        data.positions.push_back({ aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z, 1 });
        data.normals.push_back({ aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z, 0 });
        data.tangents.push_back({ aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z, 0 });
        data.textureCoords.push_back({ aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y });
    }
    for(unsigned i = 0; i < aimesh->mNumFaces; ++i) {
        aiFace face = aimesh->mFaces[i];
        for(unsigned j = 0; j < face.mNumIndices; ++j) {
            data.indices.push_back(face.mIndices[j]);
        }
    }
}

void loadMaterialTextures(std::vector<opengl::Texture> &textures, aiMaterial *material, aiTextureType const type, std::string const &typeName, int flags, std::vector<std::pair<std::string, opengl::Texture>> &loadedTextureCache, std::filesystem::path const &textureDirectory)
{
    aiString str;
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        material->GetTexture(type, i, &str);
        bool alreadyLoaded = false;
        for(auto &loadedTexturePair : loadedTextureCache) {
            if(loadedTexturePair.first == textureDirectory.string() + '/' + str.C_Str()) {
                textures.push_back(loadedTexturePair.second);
                alreadyLoaded = true;
                break;
            }
        }
        if(!alreadyLoaded) {
            std::string filepath{textureDirectory.string() + '/' + str.C_Str()};
            std::replace_if(filepath.begin(), filepath.end(), [](char c){ return c == '\\'; }, '/');
            opengl::Texture texture{filepath, (flags & model::FLIP_TEXTURES) != 0, type == aiTextureType_DIFFUSE, GL_NEAREST, GL_CLAMP_TO_EDGE, typeName};
            texture.type = typeName;
            textures.push_back(texture);
            loadedTextureCache.push_back(std::make_pair(filepath, texture));
        }
    }
}
model::Mesh model::Model::processMesh(aiMesh const *aimesh, int flags, aiScene const *scene)
{
    assert(aimesh->HasTextureCoords(0));
    assert(aimesh->HasNormals());
    assert(aimesh->HasTangentsAndBitangents());

    Mesh mesh{};
    mesh.data.emplace();
    extractVertexData(mesh.data.value(), aimesh, scene);
    if(aimesh->HasBones()) {
        extractBones(mesh.data.value(), m_boneMap, m_tposeTransform, aimesh, scene);
        m_boneTransformations.resize(m_tposeTransform.size());
    }

    if(flags & LOAD_DRAWABLE) {
        mesh.drawable.emplace();
        makeDrawable(mesh.drawable.value(), mesh.data.value());
    }

    if(aimesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[aimesh->mMaterialIndex];
        loadMaterialTextures(mesh.textures, material, aiTextureType_DIFFUSE,  "diffuse",  flags, m_loadedTextures, m_directory);
        loadMaterialTextures(mesh.textures, material, aiTextureType_SPECULAR, "specular", flags, m_loadedTextures, m_directory);
        loadMaterialTextures(mesh.textures, material, aiTextureType_HEIGHT,   "normal",   flags, m_loadedTextures, m_directory);
        loadMaterialTextures(mesh.textures, material, aiTextureType_NORMALS,  "normal",   flags, m_loadedTextures, m_directory);
    }

    if(!(flags & LOAD_DATA)) { // deallocate data
        mesh.data = {};
    }

    return mesh;
}

model::Model::Model(std::filesystem::path const &filePath, int flags)
{
    m_importer = std::make_shared<Assimp::Importer>();
    m_scene = m_importer->ReadFile( filePath.string().c_str(),
        aiProcess_GenNormals            |
        aiProcess_CalcTangentSpace      |
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType           |
        aiProcess_OptimizeGraph         |
        aiProcess_OptimizeMeshes        |
        ((flags & FLIP_WINDING_ORDER) ? aiProcess_FlipWindingOrder : 0)
    );
    if(!m_scene) {
        std::cout << "error parcing " << filePath << " model:\n\t" << m_importer->GetErrorString() << "\n";
    } 
    assert(m_scene);

    m_directory = filePath.parent_path();
    m_globalInverseTransorm = toMat4(m_scene->mRootNode->mTransformation.Inverse());
    processNode(m_scene->mRootNode, flags, m_scene);
}

std::vector<glm::mat4> const &model::Model::getBoneTransformations(float animationTimeSeconds, aiAnimation const *animation)
{
    float ticksPerSecond = (float) (animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
    float timeTicks = animationTimeSeconds * ticksPerSecond;
    return getBoneTransformations(animation, timeTicks);
}
std::vector<glm::mat4> const &model::Model::getBoneTransformations(float firstTimeSeconds, float secondTimeSeconds, aiAnimation const *first, aiAnimation const *second, float factor)
{
    float firstTicksPerSecond = (float) (first->mTicksPerSecond != 0 ? first->mTicksPerSecond : 25.0f);
    float secondTicksPerSecond = (float) (second->mTicksPerSecond != 0 ? second->mTicksPerSecond : 25.0f);
    float firstTimeTicks = firstTimeSeconds * firstTicksPerSecond;
    float secondTimeTicks = secondTimeSeconds * secondTicksPerSecond;
    return getBoneTransformations(first, second, factor, firstTimeTicks, secondTimeTicks);
}

std::vector<glm::mat4> const &model::Model::getBoneTransformations(aiAnimation const *animation, float animationTimeTicks)
{
    assert(animationTimeTicks <= animation->mDuration);
    processAnimationNode(getScene()->mRootNode, animation, animationTimeTicks, nullptr, 0, 0, glm::mat4{1.0f}, m_globalInverseTransorm, m_boneMap, m_boneTransformations, m_tposeTransform);
    return m_boneTransformations;
}
std::vector<glm::mat4> const &model::Model::getBoneTransformations(aiAnimation const *first, aiAnimation const *second, float factor, float firstTimeTicks, float secondTimeTicks)
{
    assert(firstTimeTicks <= first->mDuration);
    assert(secondTimeTicks <= second->mDuration);
    processAnimationNode(getScene()->mRootNode, first, firstTimeTicks, second, secondTimeTicks, factor, glm::mat4{1.0f}, m_globalInverseTransorm, m_boneMap, m_boneTransformations, m_tposeTransform);
    return m_boneTransformations;
}
