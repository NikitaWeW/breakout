#include "detail.hpp"
#include "engine/DSA/Data.hpp"
#include "ogl.hpp"
#include "stb_rect_pack.h"
#include <glm/trigonometric.hpp>

// FIXME: This file contains shittiest spaghetti code on planet earth that needs to be nuked.
// Shut it up: 
// #define RENDERER_TRACE(...) void(0)

using namespace engine;

constexpr std::array<glm::mat4, 6> CUBE_FACE_MATRICES = {
    glm::mat4{ { 0, 0,-1, 0 }, { 0, 1, 0, 0 }, {-1, 0, 0, 0 }, { 0, 0, 0, 1 } }, // +X
    glm::mat4{ { 0, 0, 1, 0 }, { 0, 1, 0, 0 }, { 1, 0, 0, 0 }, { 0, 0, 0, 1 } }, // -X
    glm::mat4{ { 1, 0, 0, 0 }, { 0, 0,-1, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0, 1 } }, // +Y
    glm::mat4{ { 1, 0, 0, 0 }, { 0, 0, 1, 0 }, { 0,-1, 0, 0 }, { 0, 0, 0, 1 } }, // -Y
    glm::mat4{ {-1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0,-1, 0 }, { 0, 0, 0, 1 } }, // +Z
    glm::mat4{ { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }, // -Z
};
static glm::vec3 getDir(glm::quat q)
{
    return glm::rotate(q, {0, 0, -1});
}
static glm::vec3 getUp(glm::vec3 dir)
{
    return glm::abs(glm::dot(dir, {0,1,0})) >= 0.999 ? glm::vec3{1, 0, 0} : glm::vec3{0, 1, 0};
}

bool renderer::LightManager::isShadowLightChanged(ecs::entity index, engine::ShadowLight const &light)
{
    bool isNew = mShadowLightsCache.contains(index);
    auto &cached = mShadowLightsCache[index];
    return cached != light && !isNew;
}
void renderer::LightManager::processPointLight(Entity e_light)
{
    RENDERER_TRACE("Processing point light e{}", e_light.entity());
    auto const &light = e_light.get<engine::PointLight>();
    auto const index = e_light.entity();

    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};
    renderer::PointLight newLight{
        .color = light.color,
        .position = transform.position,
        .farPlane = 0.0f,
    };
    if(mPointLights.contains(index))
        newLight.location = mPointLights.get(index).location;

    if(e_light.has<engine::ShadowLight>())
    {
        mDrawLightChanged = true;
        auto const &shadowLight = e_light.get<engine::ShadowLight>();

        if(isShadowLightChanged(index, shadowLight))
            mViewportChanged = true;
            
        newLight.farPlane = shadowLight.farPlane;

        mShadowLights[index] = {
            .size = shadowLight.shadowMapSize,
            .viewMat = glm::translate({1.0f}, -transform.position),
            .projMat = glm::perspective(glm::radians(90.0f), 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
        };
    }
    mPointLights[index] = std::move(newLight);
}
void renderer::LightManager::processDirLight(Entity e_light)
{
    RENDERER_TRACE("Processing dir light e{}", e_light.entity());
    auto const index = e_light.entity();
    auto const &light = e_light.get<engine::DirectionalLight>();
    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};
    renderer::DirLight newLight{
        .color = light.color,
        .direction = getDir(transform.orientation)
    };
    if(mDirLights.contains(index))
        newLight.location = mDirLights.get(index).location;
    
    if(e_light.has<engine::ShadowLight>())
    {
        mDrawLightChanged = true;
        auto const &shadowLight = e_light.get<engine::ShadowLight>();
        if(isShadowLightChanged(index, shadowLight))
            mViewportChanged = true;
        
        // TODO: cascade SM

        auto const &cameraTransform = mECamera.get<engine::Transform>();
        mShadowLights[index] = {
            .size = shadowLight.shadowMapSize,
            .viewMat = glm::lookAt(glm::vec3{cameraTransform.position.x, 0, cameraTransform.position.z} - (newLight.direction * 50.0f), glm::vec3{cameraTransform.position.x, 0, cameraTransform.position.z}, getUp(newLight.direction)),
            .projMat = glm::ortho<float>(-40, 40, -40, 40, shadowLight.nearPlane, shadowLight.farPlane),
        };
        newLight.viewProj = mShadowLights.get(index).projMat * mShadowLights.get(index).viewMat;
    }
    mDirLights[index] = std::move(newLight);
}
void renderer::LightManager::processSpotLight(Entity e_light)
{
    RENDERER_TRACE("Processing spot light e{}", e_light.entity());
    auto const index = e_light.entity();
    auto const &light = e_light.get<engine::SpotLight>();
    Transform transform = e_light.has<Transform>() ? e_light.get<Transform>() : Transform{};

    renderer::SpotLight newLight{
        .color = light.color,
        .position = transform.position,
        .direction = getDir(transform.orientation),
        .innerConeAngle = glm::cos(glm::radians(light.innerConeAngle)),
        .outerConeAngle = glm::cos(glm::radians(light.outerConeAngle))
    };
    if(mDirLights.contains(index))
        newLight.location = mDirLights.get(index).location;

    if(e_light.has<engine::ShadowLight>())
    {
        mDrawLightChanged = true;
        auto const &shadowLight = e_light.get<engine::ShadowLight>();
        if(isShadowLightChanged(index, shadowLight))
            mViewportChanged = true;
        
        mShadowLights[index] = {
            .size = shadowLight.shadowMapSize,
            .viewMat = glm::lookAt(newLight.position, newLight.position + newLight.direction, getUp(newLight.direction)),
            .projMat = glm::perspective(glm::radians(light.outerConeAngle * 2), 1.0f, shadowLight.nearPlane, shadowLight.farPlane),
        };
        newLight.viewProj = mShadowLights.get(index).projMat * mShadowLights.get(index).viewMat;
    }
    mSpotLights[index] = std::move(newLight);
}
void renderer::LightManager::deleteLight(ecs::entity light)
{
    RENDERER_TRACE("Removing light e{}", light);
    if(mShadowLightsCache.contains(light))
        mViewportChanged = true;

    mVersions.erase(light);
    if(mPointLights.contains(light))       mPointLights.erase(light);
    if(mDirLights.contains(light))         mDirLights.erase(light);
    if(mSpotLights.contains(light))        mSpotLights.erase(light);
    if(mShadowLights.contains(light))      mShadowLights.erase(light);
    if(mShadowLightsCache.contains(light)) mShadowLightsCache.erase(light);

    mShouldUpdate = true;
}
bool renderer::LightManager::isOmnidirectional(ecs::entity e)
{
    return mPointLights.contains(e);
}
void renderer::LightManager::makeAtlas()
{
    std::vector<stbrp_rect> rects;
    rects.reserve(mAtlas.viewports.size());

    for(auto [index, light] : mShadowLights)
    {
        rects.emplace_back(stbrp_rect{
            .id = static_cast<int>(index),
            .w = static_cast<int>(light.size) * (isOmnidirectional(index) ? 6 : 1),
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

    ogl::attachment(mAtlas.fbo, mAtlas.texture, mAtlas.size, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32);
    mAtlas.framesDrawn = 0;
    mAtlas.refreshed = true;

    ENGINE_CORE_TRACE("Made {}x{} mAtlas", mAtlas.size.x, mAtlas.size.y);

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
}
/* void renderer::LightManager::sortViewports()
{
    RENDERER_TRACE("Sorting {} viewports", mAtlas.viewports.size());
    RENDERER_TRACE("Viewport index to entity: {}", mAtlas.viewportIndexToEntity);
    RENDERER_TRACE("Entity to draw light index: {}", mAtlas.entityToDrawLightIndex);

    std::vector<size_t> indices(mAtlas.viewports.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](auto const &first, auto const &second){
        auto firstIndex = mAtlas.entityToDrawLightIndex.at(mAtlas.viewportIndexToEntity.at(first));
        auto secondIndex = mAtlas.entityToDrawLightIndex.at(mAtlas.viewportIndexToEntity.at(second));

        return firstIndex == secondIndex ? first < second : firstIndex < secondIndex;
    });
    
    RENDERER_TRACE("Applying {} indices: {}", indices.size(), indices);
    auto const viewportsCopy = mAtlas.viewports;
    auto const viewportsToEntityCopy = mAtlas.viewportIndexToEntity;
    mAtlas.viewportIndexToEntity.clear();
    for(size_t i = 0; i < indices.size(); ++i)
    {
        auto index = indices[i];
        mAtlas.viewports[i] = viewportsCopy[index];
        mAtlas.viewportIndexToEntity[i] = viewportsToEntityCopy.at(index);
        RENDERER_TRACE("{})\tindex: {}, \t e{}", i, index, viewportsToEntityCopy.at(index));
    }


    RENDERER_TRACE("Result: ");
    for(size_t i = 0; i < mAtlas.viewports.size(); ++i)
    {
        auto const &viewport = mAtlas.viewports[i];
        auto entity = mAtlas.viewportIndexToEntity.at(i);
        auto index = mAtlas.entityToDrawLightIndex.at(mAtlas.viewportIndexToEntity.at(i));
        RENDERER_TRACE("Position: [{}, {}], \tsize: [{}, {}], \t viewport index: {}, \tdraw light index: {}, \t e{}", viewport.pos.x, viewport.pos.y, viewport.size.x, viewport.size.y, i, index, entity);
    }
    RENDERER_TRACE("================");

} */
void renderer::LightManager::makeDrawLights()
{
    RENDERER_TRACE("Making {} draw lights", mShadowLights.size());

    mAtlas.drawLights.clear();
    for(auto [index, light] : mShadowLights)
    {
        if(isOmnidirectional(index))
        {
            auto const &pointLight = mPointLights.get(index);
            for(unsigned i = 0; i < 6; ++i)
            {
                mAtlas.drawLights.emplace_back(DrawLight{
                    .viewMat = CUBE_FACE_MATRICES[i] * light.viewMat,
                    .projMat = light.projMat,
                    .omnidirectional = true,
                    .farPlane = pointLight.farPlane,
                    .position = pointLight.position
                });
            }
        } else {
            mAtlas.drawLights.emplace_back(DrawLight{
                .viewMat = light.viewMat,
                .projMat = light.projMat,
                .omnidirectional = false
            });
        }
    }
}
void renderer::LightManager::setCamera(engine::Entity e_cam)
{
    mECamera = e_cam;
}
void renderer::LightManager::setup()
{
    glCreateFramebuffers(1, &mAtlas.fbo.id);
    {
        std::array<GLenum, 1> const drawbuffers = { GL_NONE };
        glNamedFramebufferDrawBuffers(mAtlas.fbo.id, drawbuffers.size(), drawbuffers.data());
    }
    glCreateBuffers(1, &mPointLightsSSBO.id);
    glCreateBuffers(1, &mDirLightsSSBO.id);
    glCreateBuffers(1, &mSpotLightsSSBO.id);
    glCreateBuffers(1, &mAtlas.drawLightsSSBO.id);
}

void renderer::LightManager::tryUpdateLight(Entity light)
{
    if(!light.has<Version>())
    {
        ENGINE_CORE_ERROR("renderer::LightManager::updateLight: e{} doesent have a \"Version\" component", light.entity());
        return;
    }
    mThisFrameLights.emplace(light.entity());
    auto &version = light.get<Version>();

    bool isNew = !mVersions.contains(light.entity());
    auto &currentVersion = mVersions[light.entity()];
    if(currentVersion == version && !isNew)
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

void renderer::LightManager::populateBuffers()
{
    RENDERER_TRACE("Populating lighting buffers. {} point lights, {} dir lights, {} spot lights, {} draw lights.", mPointLights.size(), mDirLights.size(), mSpotLights.size(), mAtlas.drawLights.size());
    glNamedBufferData(mPointLightsSSBO.id,       mPointLights.size()      * sizeof(renderer::PointLight), mPointLights.data().data(), GL_STATIC_DRAW);
    glNamedBufferData(mDirLightsSSBO.id,         mDirLights.size()        * sizeof(renderer::DirLight),   mDirLights.data().data(),   GL_STATIC_DRAW);
    glNamedBufferData(mSpotLightsSSBO.id,        mSpotLights.size()       * sizeof(renderer::SpotLight),  mSpotLights.data().data(),  GL_STATIC_DRAW);
    glNamedBufferData(mAtlas.drawLightsSSBO.id,  mAtlas.drawLights.size() * sizeof(renderer::DrawLight),  mAtlas.drawLights.data(),   GL_STATIC_DRAW);
}

void renderer::LightManager::apply()
{
    for(auto e_light : mLastFrameLights)
    {
        if(mThisFrameLights.find(e_light) == mThisFrameLights.end())
            deleteLight(e_light);
    }
    mLastFrameLights = mThisFrameLights;
    mThisFrameLights.clear();

    if(!mShouldUpdate)
        return;

    mAtlas.refreshed = false;
    if(mViewportChanged)
    {
        makeAtlas();
        // sortViewports();
    }
    if(mViewportChanged || mDrawLightChanged)
    {
        makeDrawLights();
    }
    populateBuffers();

    mViewportChanged = false;
    mDrawLightChanged = false;
    mShouldUpdate = false;
}
