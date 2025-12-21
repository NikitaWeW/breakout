#include "engine/Header/Config.hpp"
#include "engine/Logging/Logging.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "detail.hpp"
#include "ogl.hpp"
#include "stb_rect_pack.h"
#include <nicecs/ecs.hpp>

// TODO: versioning system: Only update / add objects if the versions mismatch.

namespace ogl = engine::ogl;
using namespace engine;

void EngineRendererImpl::processModels()
{
    using namespace engine;
    for(ecs::entity e_model : data.reg->view<Model>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        auto const &model = data.reg->get<Model>(e_model);
        renderer::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            renderer::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.count = mesh.geometry.indices.size();
    
            newMesh.material = renderer::convertMaterial(*data.reg, mesh.material);
    
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

        data.reg->emplace<renderer::Model>(e_model, std::move(newModel));
        data.reg->emplace<renderer::ProcessedTag>(e_model);
    }
}
void EngineRendererImpl::processTextures()
{
    for(ecs::entity e_texture : data.reg->view<Texture>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        auto const &texture = data.reg->get<Texture>(e_texture);

        data.reg->emplace<ogl::Texture>(e_texture, ogl::makeTexture(texture.data, texture.srgb));
        data.reg->emplace<renderer::ProcessedTag>(e_texture);
    }

    for(ecs::entity e_cubemap : data.reg->view<Cubemap>(ecs::exclude_t<renderer::ProcessedTag>{}))
    {
        auto const &cubemap = data.reg->get<Cubemap>(e_cubemap);

        data.reg->emplace<ogl::Cubemap>(e_cubemap, ogl::makeCubemap(cubemap.faces));
        data.reg->emplace<renderer::ProcessedTag>(e_cubemap);
    }
}

static glm::vec3 getDir(glm::quat q)
{
    return glm::rotate(q, {0, 0, -1});
}
static glm::vec3 getUp(glm::vec3 dir)
{
    return glm::abs(glm::dot(dir, {0,1,0})) >= 0.99 ? glm::vec3{1, 0, 0} : glm::vec3{0, 1, 0};
}

// TODO: process data automatically based on the versioning system.
void EngineRendererImpl::processData()
{
    processTextures();
    processModels();

    for(auto e_light : data.reg->view<DynamicLight>())
    {
        data.mLightManager.tryUpdateLight(Entity{*data.reg, e_light});
    }
}

void renderer::LightManager::processPointLight(Entity e_light)
{
    auto const &light = e_light.get<PointLight>();
    auto index = e_light.entity();

    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};
    renderer::PointLight newLight{
        .color = light.color,
        .position = transform.position,
        .farPlane = 0.0f,
    };

    if(e_light.has<ShadowLight>())
    {
        auto const &shadowLight = e_light.get<ShadowLight>();
        newLight.farPlane = shadowLight.farPlane;

        // No idea if this is correct...k
        // Edit: looks like it is correct...
        constexpr std::array<glm::mat4, 6> CUBE_FACE_MATRICES = {
            glm::mat4{ { 0.0f,  0.0f, -1.0f, 0.0f }, { 0.0f, -1.0f,  0.0f, 0.0f }, {-1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }, // +X
            glm::mat4{ { 0.0f,  0.0f,  1.0f, 0.0f }, { 0.0f, -1.0f,  0.0f, 0.0f }, { 1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }, // -X
            glm::mat4{ { 1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  1.0f, 0.0f }, { 0.0f, -1.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }, // +Y
            glm::mat4{ { 1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f,  0.0f, -1.0f, 0.0f }, { 0.0f,  1.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }, // -Y
            glm::mat4{ { 1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f, -1.0f,  0.0f, 0.0f }, { 0.0f,  0.0f, -1.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }, // +Z
            glm::mat4{ {-1.0f,  0.0f,  0.0f, 0.0f }, { 0.0f, -1.0f,  0.0f, 0.0f }, { 0.0f,  0.0f,  1.0f, 0.0f }, { 0.0f,  0.0f,  0.0f, 1.0f } }  // -Z
        };

        for(unsigned i = 0; i < 6; ++i)
        {
            mDrawLights[index] = renderer::DrawLight{
                .viewMat = CUBE_FACE_MATRICES[i] * glm::translate({1.0f}, -transform.position),
                .projMat = glm::perspective(glm::radians(90.0f), 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
            };
            mViewports[index] = Viewport{
                .size = shadowLight.shadowMapSize
            };
        }
    }
    mPointLights[index] = std::move(newLight);
}
void renderer::LightManager::processDirLight(Entity e_light)
{
    auto index = e_light.entity();
    auto const &light = e_light.get<DirectionalLight>();
    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};
    renderer::DirLight newLight{
        .color = light.color,
        .direction = getDir(transform.orientation)
    };
    
    if(e_light.has<ShadowLight>())
    {
        auto const &shadowLight = e_light.get<ShadowLight>();
        
        // TODO: cascade SM
        mDrawLights[index] = renderer::DrawLight{
            .viewMat = glm::lookAt(glm::vec3{0} - (newLight.direction * 10.0f), glm::vec3{0}, getUp(newLight.direction)),
            .projMat = glm::ortho<float>(-10, 10, -10, 10, shadowLight.nearPlane, shadowLight.farPlane),
        };
        mViewports[index] = Viewport{
            .size = shadowLight.shadowMapSize
        };
        newLight.viewProj = mDrawLights.data().back().projMat * mDrawLights.data().back().viewMat;
    }
    mDirLights[index] = std::move(newLight);
}
void renderer::LightManager::processSpotLight(Entity e_light)
{
    auto index = e_light.entity();
    auto const &light = e_light.get<SpotLight>();
    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};

    renderer::SpotLight newLight{
        .color = light.color,
        .position = transform.position,
        .direction = getDir(transform.orientation),
        .innerConeAngle = light.innerConeAngle,
        .outerConeAngle = light.outerConeAngle
    };

    if(e_light.has<ShadowLight>())
    {
        auto const &shadowLight = e_light.get<ShadowLight>();
        mDrawLights[index] = renderer::DrawLight{
            .viewMat = glm::lookAt(newLight.position, newLight.position + newLight.direction, getUp(newLight.direction)),
            .projMat = glm::perspective(glm::radians(newLight.outerConeAngle), 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
        };
        newLight.viewProj = mDrawLights.data().back().projMat * mDrawLights.data().back().viewMat;
        mViewports[index] = Viewport{
            .size = shadowLight.shadowMapSize
        };
    }
    mSpotLights[index] = std::move(newLight);
}
void renderer::LightManager::makeAtlas()
{
    std::vector<stbrp_rect> rects;
    rects.reserve(mAtlas.viewports.size());

    for(auto [index, viewport] : mViewports)
    {
        rects.emplace_back(stbrp_rect{
            .id = static_cast<int>(index),
            .w = static_cast<int>(viewport.size) * (mPointLights.contains(index) ? 6 : 1),
            .h = static_cast<int>(viewport.size),
            .x = 0,
            .y = 0,
        });
    }

    unsigned maxWidth = std::max_element(rects.begin(), rects.end(), [](auto const &a, auto const &b){ return a.w < b.w; })->w;
    std::vector<stbrp_node> nodes(maxWidth);
    stbrp_context context;
    stbrp_init_target(&context, maxWidth, std::numeric_limits<int>::max(), nodes.data(), nodes.size());
    stbrp_setup_allow_out_of_mem(&context, true);
    stbrp_pack_rects(&context, rects.data(), rects.size());

    mAtlas.size = {0, 0};
    mAtlas.viewports.clear();
    for(auto const &rect : rects)
    {
        auto index = rect.id;
        
        mAtlas.size = glm::max(mAtlas.size, {rect.x + rect.w, rect.y + rect.h});

        renderer::AtlasLocation location{
            .pos = { rect.x, rect.y },
            .size = static_cast<unsigned>(rect.h) 
        };
        renderer::DrawLightViewport viewport{
            .pos = location.pos,
            .size = glm::uvec2{ location.size }
        };
        viewport.pos = location.pos;

        if(mPointLights.contains(index)) {
            mPointLights[index].location = location;
            for(size_t i = 0; i < 6; ++i)
            {
                mAtlas.viewports.emplace_back(viewport);
                location.pos.x += location.size;
                viewport.pos.x += viewport.size.x;
            }
        } else if(mDirLights.contains(index)) {
            mDirLights[index].location = std::move(location);
            mAtlas.viewports.emplace_back(std::move(viewport));
        } else if(mSpotLights.contains(index)) {
            mSpotLights[index].location = std::move(location);
            mAtlas.viewports.emplace_back(std::move(viewport));
        } else {
            ENGINE_ASSERT_MSG(false, "how did we get here?");
        }
    }


    RENDERER_TRACE("Made {}x{} mAtlas", mAtlas.size.x, mAtlas.size.y);

    for(auto const &light : mPointLights.data())
        ENGINE_CORE_TRACE("Point light.       Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    for(auto const &light : mDirLights.data())
        ENGINE_CORE_TRACE("Directional light. Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    for(auto const &light : mSpotLights.data())
        ENGINE_CORE_TRACE("Spot light.        Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    ENGINE_CORE_TRACE("================");
    ENGINE_CORE_TRACE("{} viewports: ", mAtlas.viewports.size());
    for(auto const &viewport : mAtlas.viewports)
        ENGINE_CORE_TRACE("Position: [{}, {}], \tsize: [{}, {}]", viewport.pos.x, viewport.pos.y, viewport.size.x, viewport.size.y);
    ENGINE_CORE_TRACE("================");


    ogl::attachment(mAtlas.fbo, mAtlas.texture, mAtlas.size, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24);
}

void renderer::LightManager::tryUpdateLight(Entity light)
{
    if(!light.has<Version>())
    {
        ENGINE_CORE_ERROR("renderer::LightManager::updateLight: e{} doesent have a \"Version\" component", light.entity());
        return;
    }
    auto &version = light.get<Version>();

    auto &currentVersion = mVersions[light.entity()];
    if(currentVersion == version)
        return; // Up-to-date

    currentVersion = version;
    mShouldUpdate = true;

    if(light.has<engine::PointLight>())
        processPointLight(light);
    else if(light.has<engine::DirectionalLight>())
        processDirLight(light);
    else if(light.has<engine::SpotLight>())
        processSpotLight(light);
    else if(light.has<engine::AreaLight>())
        ENGINE_ASSERT_MSG(false, "Area lights not supported yet!"); // TODO: Area lights
    else
        ENGINE_ASSERT_MSG(false, "Entity has no lights!");
}
void renderer::LightManager::apply()
{
    if(!mShouldUpdate)
        return;

    makeAtlas();

    glNamedBufferData(mPointLightsSSBO.id, mPointLights.size() * sizeof(renderer::PointLight), mPointLights.data().data(), GL_STATIC_DRAW);
    glNamedBufferData(mDirLightsSSBO.id,   mDirLights.size()   * sizeof(renderer::DirLight),   mDirLights.data().data(),   GL_STATIC_DRAW);
    glNamedBufferData(mSpotLightsSSBO.id,  mSpotLights.size()  * sizeof(renderer::SpotLight),  mSpotLights.data().data(),  GL_STATIC_DRAW);
    glNamedBufferData(mDrawLightsSSBO.id,  mDrawLights.size()  * sizeof(renderer::DrawLight),  mDrawLights.data().data(),  GL_STATIC_DRAW);    

    mShouldUpdate = false;
}
