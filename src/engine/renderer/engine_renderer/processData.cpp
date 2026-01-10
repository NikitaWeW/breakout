#include "engine/DSA/ECS.hpp"
#include "detail.hpp"

using namespace engine;
// Shut it up: 
// #define RENDERER_TRACE(...)

void EngineRendererImpl::processModels()
{
    using namespace engine;
    for(ecs::entity e_model : mReg->view<Model>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        RENDERER_TRACE("Processing model e{}", e_model);
        auto const &model = mReg->get<Model>(e_model);
        renderer::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            renderer::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.count = mesh.geometry.indices.size();
    
            newMesh.material = renderer::convertMaterial(*mReg, mesh.material);
    
            glCreateVertexArrays(1, &newMesh.vao.id);
            
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.positions), 4, GL_FLOAT);
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.texCoords), 2, GL_FLOAT);
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.normals),   4, GL_FLOAT);
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.tangents),  4, GL_FLOAT);
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.boneIDs),   4, GL_FLOAT);
            ogl::pushVertexBuffer(newMesh.vao, ogl::makeBuffer<ogl::VBO>(mesh.geometry.weights),   4, GL_FLOAT);
            
            newMesh.ibo = ogl::makeBuffer<ogl::IBO>(mesh.geometry.indices);
            glVertexArrayElementBuffer(newMesh.vao.id, newMesh.ibo.id);

            newModel.meshes.emplace_back(std::move(newMesh));
        }

        mReg->emplace<renderer::Model>(e_model, std::move(newModel));
        mReg->emplace<renderer::ProcessedTag>(e_model);
    }
}
void EngineRendererImpl::processTextures()
{
    for(ecs::entity e_texture : mReg->view<Texture>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        RENDERER_TRACE("Processing texture e{}", e_texture);
        auto const &texture = mReg->get<Texture>(e_texture);

        mReg->emplace<ogl::Texture>(e_texture, ogl::makeTexture(texture.data, texture.srgb));
        mReg->emplace<renderer::ProcessedTag>(e_texture);
    }

    for(ecs::entity e_cubemap : mReg->view<Cubemap>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        RENDERER_TRACE("Processing cubemap e{}", e_cubemap);
        auto const &cubemap = mReg->get<Cubemap>(e_cubemap);

        mReg->emplace<ogl::Cubemap>(e_cubemap, ogl::makeCubemap(cubemap.faces));
        mReg->emplace<renderer::ProcessedTag>(e_cubemap);
    }
}

// TODO: process data automatically based on the versioning system.
void EngineRendererImpl::processData()
{
    processTextures();
    processModels();

    for(auto e_light : mReg->viewAny<engine::PointLight, engine::DirectionalLight, engine::SpotLight, engine::AreaLight>())
        mLightManager.tryUpdateLight(Entity{*mReg, e_light});

    mLightManager.apply();
}


