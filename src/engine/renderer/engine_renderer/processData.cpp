#include "engine/DSA/ECS.hpp"
#include "detail.hpp"

using namespace engine;
// Shut it up: 
// #define RENDERER_TRACE(...)

void EngineRendererImpl::processModels()
{
    using namespace engine;
    for(ecs::entity e_model : reg->view<Model>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        RENDERER_TRACE("Processing model e{}", e_model);
        auto const &model = reg->get<Model>(e_model);
        renderer::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            renderer::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.count = mesh.geometry.indices.size();
    
            newMesh.material = renderer::convertMaterial(*reg, mesh.material);
    
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

        reg->emplace<renderer::Model>(e_model, std::move(newModel));
        reg->emplace<renderer::ProcessedTag>(e_model);
    }
}
void EngineRendererImpl::processTextures()
{
    for(ecs::entity e_texture : reg->view<Texture>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        RENDERER_TRACE("Processing texture e{}", e_texture);
        auto const &texture = reg->get<Texture>(e_texture);

        reg->emplace<ogl::Texture>(e_texture, ogl::makeTexture(texture.data, texture.srgb));
        reg->emplace<renderer::ProcessedTag>(e_texture);
    }

    for(ecs::entity e_cubemap : reg->view<Cubemap>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        auto const &cubemap = reg->get<Cubemap>(e_cubemap);

        reg->emplace<ogl::Cubemap>(e_cubemap, ogl::makeCubemap(cubemap.faces));
        reg->emplace<renderer::ProcessedTag>(e_cubemap);
    }
}

// TODO: process data automatically based on the versioning system.
void EngineRendererImpl::processData()
{
    processTextures();
    processModels();

    for(auto e_light : reg->viewAny<engine::PointLight, engine::DirectionalLight, engine::SpotLight, engine::AreaLight>())
        mLightManager.tryUpdateLight(Entity{*reg, e_light});

    mLightManager.apply();
}


