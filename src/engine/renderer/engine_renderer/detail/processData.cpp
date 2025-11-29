#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "detail.hpp"
#include "ogl.hpp"
#include "stb_rect_pack.h"

// TODO: versioning system: Only update / add objects if the versions mismatch.

namespace ogl = engine::renderer::ogl;

static ogl::Texture getTexture(ecs::registry const &reg, ecs::entity e_texture)
{
    using namespace engine;
    if(e_texture == 0)
        return ogl::Texture{};
    ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedTexture>(e_texture), "Forgot to call engine::EngineRenderer::processData?");

    return reg.get<ogl::Texture>(e_texture);
}
void engine::EngineRenderer::processModels(ecs::registry &reg)
{
    using namespace engine;
    for(ecs::entity e_model : reg.view<engine::Model>(ecs::exclude_t<renderer::ProcessedModel>{}))
    {
        auto const &model = reg.get<engine::Model>(e_model);
        renderer::Model newModel;
        newModel.skeleton = model.skeleton;
        newModel.animated = model.skeleton.boneMap.size() != 0;

        for(auto const &mesh : model.meshes)
        {
            ENGINE_ASSERT_MSG(reg.has<renderer::ProcessedMaterial>(mesh.e_material), "Forgot to call engine::EngineRenderer::processData?");
            renderer::Mesh newMesh;
            newMesh.mode = GL_TRIANGLES;
            newMesh.count = mesh.geometry.indices.size();
    
            newMesh.e_material = mesh.e_material;
    
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

        reg.emplace<renderer::Model>(e_model, std::move(newModel));
        reg.emplace<renderer::ProcessedModel>(e_model);
    }
}
void engine::EngineRenderer::processMaterials(ecs::registry &reg) 
{
    for(ecs::entity e_material : reg.view<engine::Material>(ecs::exclude_t<renderer::ProcessedMaterial>{}))
    {
        auto const &material = reg.get<engine::Material>(e_material);
        ecs::entity const &entity = e_material; // destination

        renderer::Material newMaterial;
        newMaterial.properties = material.properties;
        newMaterial.textures = {
            .albedo    = getTexture(reg, material.textures.albedo),
            .normal    = getTexture(reg, material.textures.normal),
            .metallic  = getTexture(reg, material.textures.metallic),
            .roughness = getTexture(reg, material.textures.roughness) 
        };

        reg.emplace<renderer::Material>(entity, std::move(newMaterial));
        reg.emplace<renderer::ProcessedMaterial>(e_material);
    }
}
void engine::EngineRenderer::processTextures(ecs::registry &reg)
{
    for(ecs::entity e_texture : reg.view<engine::Texture>(ecs::exclude_t<renderer::ProcessedTexture>{}))
    {
        auto const &texture = reg.get<engine::Texture>(e_texture);

        reg.emplace<ogl::Texture>(e_texture, ogl::makeTexture(texture.data, texture.srgb));
        reg.emplace<renderer::ProcessedTexture>(e_texture);
    }

    for(ecs::entity e_cubemap : reg.view<engine::Cubemap>(ecs::exclude_t<renderer::ProcessedCubemap>{}))
    {
        auto const &cubemap = reg.get<engine::Cubemap>(e_cubemap);

        reg.emplace<ogl::Cubemap>(e_cubemap, ogl::makeCubemap(cubemap.faces));
        reg.emplace<renderer::ProcessedCubemap>(e_cubemap);
    }
}

// TODO: refactor this mess

static void processPointLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::PointLight>(e_light);
    engine::renderer::PointLight newLight{
        .color = light.color,
        .position = reg.has<engine::ModelMatrix>(e_light) ? reg.get<engine::ModelMatrix>(e_light)[3] : glm::vec3{0},
        .farPlane = 0.0f,
    };

    if(reg.has<engine::ShadowLight>(e_light))
    {
        auto const &shadowLight = reg.get<engine::ShadowLight>(e_light);
        data.SMAtlas.lights.emplace_back(engine::renderer::ShadowMapAtlas::Light{
            .type = engine::renderer::ShadowMapAtlas::Light::POINT,
            .drawLightIndex = data.drawLights.size(),
            .lightIndex = data.pointLights.size(),
            .size = shadowLight.shadowMapSize
        });

        constexpr std::array<glm::mat4, 6> POINT_CUBE_FACE_MATRICES = {
            glm::mat4{ { 0 ,0,-1, 0 }, { 0,-1, 0, 0 }, {-1, 0, 0,0 }, {0, 0, 0, 1} },
            glm::mat4{ { 0 ,0, 1, 0 }, { 0,-1, 0, 0 }, { 1, 0, 0,0 }, {0, 0, 0, 1} },
            glm::mat4{ { 1 ,0, 0, 0 }, { 0, 0, 1, 0 }, { 0,-1, 0,0 }, {0, 0, 0, 1} },
            glm::mat4{ { 1 ,0, 0, 0 }, { 0, 0,-1, 0 }, { 0, 1, 0,0 }, {0, 0, 0, 1} },
            glm::mat4{ { 1 ,0, 0, 0 }, { 0,-1, 0, 0 }, { 0, 0,-1,0 }, {0, 0, 0, 1} },
            glm::mat4{ {-1 ,0, 0, 0 }, { 0,-1, 0, 0 }, { 0, 0, 1,0 }, {0, 0, 0, 1} } 
        };
        for(unsigned i = 0; i < 6; ++i)
        {
            data.drawLights.emplace_back(engine::renderer::DrawLight{
                .viewMat = POINT_CUBE_FACE_MATRICES[i] * (reg.has<engine::ModelMatrix>(e_light) ? static_cast<glm::mat4 const &>(reg.get<engine::ModelMatrix>(e_light)) : glm::mat4{1.0f}),
                .projMat = glm::perspective(glm::radians(90.0f), 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
            });
        }
    }
    data.pointLights.emplace_back(std::move(newLight));
}
static void processDirLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::DirectionalLight>(e_light);
    glm::mat4 const &modelMat = reg.has<engine::ModelMatrix>(e_light) ? static_cast<glm::mat4 const &>(reg.get<engine::ModelMatrix>(e_light)) : glm::mat4{1.0f};
    engine::renderer::DirLight newLight{
        .color = light.color,
        .direction = -glm::normalize(glm::vec3(modelMat[2]))
    };
    
    if(reg.has<engine::ShadowLight>(e_light))
    {
        auto const &shadowLight = reg.get<engine::ShadowLight>(e_light);
        data.SMAtlas.lights.emplace_back(engine::renderer::ShadowMapAtlas::Light{
            .type = engine::renderer::ShadowMapAtlas::Light::DIR,
            .drawLightIndex = data.drawLights.size(),
            .lightIndex = data.dirLights.size(),
            .size = shadowLight.shadowMapSize
        });
        // TODO: cascade SM
        data.drawLights.emplace_back(engine::renderer::DrawLight{
            .viewMat = glm::translate(glm::mat4{1.0f}, -newLight.direction * 10.0f) * modelMat,
            .projMat = glm::ortho<float>(-10, 10, -10, 10, shadowLight.nearPlane, shadowLight.farPlane),
        });
    }
    data.dirLights.emplace_back(std::move(newLight));
}
static void processSpotLight(engine::renderer::RendererData &data, ecs::registry const &reg, ecs::entity e_light)
{
    auto const &light = reg.get<engine::SpotLight>(e_light);
    glm::mat4 const &modelMat = reg.has<engine::ModelMatrix>(e_light) ? static_cast<glm::mat4 const &>(reg.get<engine::ModelMatrix>(e_light)) : glm::mat4{1.0f};

    engine::renderer::SpotLight newLight{
        .color = light.color,
        .position = modelMat[3],
        .direction = -glm::normalize(glm::vec3(modelMat[2])),
        .innerConeAngle = light.innerConeAngle,
        .outerConeAngle = light.outerConeAngle
    };

    if(reg.has<engine::ShadowLight>(e_light))
    {
        auto const &shadowLight = reg.get<engine::ShadowLight>(e_light);
        data.SMAtlas.lights.emplace_back(engine::renderer::ShadowMapAtlas::Light{
            .type = engine::renderer::ShadowMapAtlas::Light::SPOT,
            .drawLightIndex = data.drawLights.size(),
            .lightIndex = data.spotLights.size(),
            .size = shadowLight.shadowMapSize
        });
        data.drawLights.emplace_back(engine::renderer::DrawLight{
            .viewMat = modelMat,
            .projMat = glm::perspective(newLight.outerConeAngle, 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
        });
    }
    data.spotLights.emplace_back(std::move(newLight));
}
static void makeAtlas(engine::renderer::RendererData &data)
{
    auto &atlas = data.SMAtlas;

    std::vector<stbrp_rect> rects;
    rects.reserve(atlas.lights.size());

    for(size_t i = 0; i < atlas.lights.size(); ++i)
    {
        auto const &light = atlas.lights[i];
        rects.emplace_back(stbrp_rect{
            .w = static_cast<int>(light.size) * (light.type == engine::renderer::ShadowMapAtlas::Light::POINT ? 6 : 1),
            .h = static_cast<int>(light.size),
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

    atlas.size = {0, 0};
    for(size_t i = 0; i < rects.size(); ++i)
    {
        auto const &light = atlas.lights[i];
        auto const &rect  = rects[i];
        
        atlas.size = glm::max(atlas.size, {rect.x + rect.w, rect.y + rect.h});

        engine::renderer::ShadowMapAtlas::Location location{
            .pos = { rect.x, rect.y },
            .size = static_cast<unsigned>(rect.h)
        };

        auto &drawLight = data.drawLights.at(light.drawLightIndex);
        drawLight.location = location;
        switch(light.type)
        {
            case light.POINT:
            data.pointLights.at(light.lightIndex).location = location;
            for(size_t i = light.drawLightIndex + 1; i < light.drawLightIndex + 6; ++i)
            {
                location.pos.x += location.size;
                data.drawLights.at(i).location = location;
            }
            break;
            case light.DIR:
            data.dirLights.at(light.lightIndex).location = location;
            break;
            case light.SPOT:
            data.spotLights.at(light.lightIndex).location = location;
            break;
        }
    }

    // ENGINE_CORE_TRACE("Made {}x{} atlas", atlas.size.x, atlas.size.y);
    // for(auto const &light : data.pointLights)
    //     ENGINE_CORE_TRACE("Point light.       Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    // for(auto const &light : data.dirLights)
    //     ENGINE_CORE_TRACE("Directional light. Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    // for(auto const &light : data.spotLights)
    //     ENGINE_CORE_TRACE("Spot light.        Position: [{}, {}], \tsize: {}", light.location.pos.x, light.location.pos.y, light.location.size);
    // ENGINE_CORE_TRACE("================");

    ogl::attachment(atlas.fbo, atlas.texture, atlas.size, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24);
}
void engine::EngineRenderer::processLights(ecs::registry const &reg, renderer::RendererData &data)
{
    data.pointLights.clear();
    data.dirLights.clear();
    data.drawLights.clear();
    data.spotLights.clear();
    data.SMAtlas.lights.clear();
    for(ecs::entity e_light : reg.view<DynamicLight>())
    {
        if(reg.has<PointLight>(e_light))
            processPointLight(data, reg, e_light);
        if(reg.has<DirectionalLight>(e_light))
            processDirLight(data, reg, e_light);
        if(reg.has<SpotLight>(e_light))
            processSpotLight(data, reg, e_light);
        if(reg.has<AreaLight>(e_light))
            ENGINE_ASSERT_MSG(false, "Area lights not implemented yet!");
    } 

    makeAtlas(data);

    glNamedBufferData(data.pointLightsSSBO.id, data.pointLights.size() * sizeof(renderer::PointLight), data.pointLights.data(), GL_STATIC_DRAW);
    glNamedBufferData(data.dirLightsSSBO.id,   data.dirLights.size()   * sizeof(renderer::DirLight),   data.dirLights.data(),   GL_STATIC_DRAW);
    glNamedBufferData(data.spotLightsSSBO.id,  data.spotLights.size()  * sizeof(renderer::SpotLight),  data.spotLights.data(),  GL_STATIC_DRAW);
    glNamedBufferData(data.drawLightsSSBO.id,  data.drawLights.size()  * sizeof(renderer::DrawLight),  data.drawLights.data(),  GL_STATIC_DRAW);
}

// TODO: process data automatically based on the versioning system.
void engine::EngineRenderer::processData(ecs::registry &reg)
{
    processTextures(reg);
    processMaterials(reg);
    processModels(reg);
}
