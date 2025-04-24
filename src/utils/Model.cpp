#include "Model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include <iostream>

void model::Model::processNode(aiNode *node, int const &flags, aiScene const *scene)
{
    for(unsigned i = 0; i < node->mNumMeshes; ++i) {
        m_meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], flags, scene));
    }
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], flags, scene);
    }
}

model::Mesh model::Model::processMesh(aiMesh *aimesh, int const &flags, aiScene const *scene)
{
    assert(aimesh->HasTangentsAndBitangents());
    assert(aimesh->HasTextureCoords(0));

    Mesh mesh{};
    mesh.data.emplace();
    MeshData &data = mesh.data.value();

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

    if(flags & LOAD_DRAWABLE) {
        mesh.drawable.emplace();
        game::Drawable &drawable = mesh.drawable.value();
        
        drawable.vb = opengl::VertexBuffer{
            data.positions.size()     * sizeof(data.positions[0]) + 
            data.normals.size()       * sizeof(data.normals[0]) + 
            data.textureCoords.size() * sizeof(data.textureCoords[0]) + 
            data.tangents.size()      * sizeof(data.tangents[0])
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

        drawable.ib = opengl::IndexBuffer{data.indices.size() * sizeof(data.indices[0]), data.indices.data()};
        drawable.va = opengl::VertexArray{drawable.vb, opengl::VertexBufferLayout{
            {4, GL_FLOAT, 0},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0])},
            {2, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0])},
            {4, GL_FLOAT, data.positions.size() * sizeof(data.positions[0]) + data.normals.size() * sizeof(data.normals[0]) + data.textureCoords.size() * sizeof(data.textureCoords[0])}
        }};
        drawable.count = data.indices.size();
    }

    if(aimesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[aimesh->mMaterialIndex];
        loadMaterialTextures(mesh.textures, material, aiTextureType_DIFFUSE,  "diffuse",  flags);
        loadMaterialTextures(mesh.textures, material, aiTextureType_SPECULAR, "specular", flags);
        loadMaterialTextures(mesh.textures, material, aiTextureType_HEIGHT,   "normal",   flags);
        loadMaterialTextures(mesh.textures, material, aiTextureType_NORMALS,  "normal",   flags);
    }

    if(!(flags & LOAD_DATA)) { // deallocate data
        mesh.data = {};
    }

    return mesh;
}

void model::Model::loadMaterialTextures(std::vector<opengl::Texture> &textures, aiMaterial *material, aiTextureType const type, std::string const &typeName, int const &flags)
{
    aiString str;
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        material->GetTexture(type, i, &str);
        bool alreadyLoaded = false;
        for(auto &loadedTexturePair : m_loadedTextures) {
            if(loadedTexturePair.first == m_directory.string() + '/' + str.C_Str()) {
                textures.push_back(loadedTexturePair.second);
                alreadyLoaded = true;
                break;
            }
        }
        if(!alreadyLoaded) {
            std::string filepath{m_directory.string() + '/' + str.C_Str()};
            std::replace_if(filepath.begin(), filepath.end(), [](char c){ return c == '\\'; }, '/');
            opengl::Texture texture{filepath, (flags & FLIP_TEXTURES) != 0, type == aiTextureType_DIFFUSE, GL_NEAREST, GL_CLAMP_TO_EDGE, typeName};
            texture.type = typeName;
            textures.push_back(texture);
            m_loadedTextures.push_back(std::make_pair(filepath, texture));
        }
    }
}

model::Model::Model(std::filesystem::path const &filePath, int flags)
{
    Assimp::Importer importer;

    aiScene const *scene = importer.ReadFile( filePath.string().c_str(),
        aiProcess_CalcTangentSpace      |
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType           |
        aiProcess_OptimizeGraph         |
        aiProcess_OptimizeMeshes        |
        ((flags & FLIP_WINDING_ORDER) ? aiProcess_FlipWindingOrder : 0)
    );

    m_directory = filePath.parent_path();
    processNode(scene->mRootNode, flags, scene);

    if (!isLoaded()) {
        std::cout << "error parcing " << filePath << " model:\n\t" << importer.GetErrorString();
    } 
}
