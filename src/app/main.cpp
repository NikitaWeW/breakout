#include <iostream>
#include <cmath>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "engine/loader/loader.hpp"
#include "controller/controller.hpp"
#include "engine/physics/physics.hpp"
#include "engine/input/input.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/io.hpp"
#include "fmt/ostream.h"
#include "cooload.hpp"
#include <thread>
#include <random>
#include <stack>

static std::string printTexture(ecs::entity e_texture, ecs::registry const &reg)
{
    std::stringstream ss;
    auto const &texture = reg.get<engine::Texture>(e_texture);
    ss 
        << 'e' << e_texture << ", \"" 
        << texture.path << "\", \t" 
        << texture.data.getDimensions() << ", \t" 
        << (texture.srgb ? "srgb" : "not srgb");
    return ss.str();
}
static void printModelData(ecs::entity e_model, ecs::registry const &registry)
{
    ENGINE_ASSERT(registry.has<engine::Model>(e_model));
    engine::Model const &model = registry.get<engine::Model>(e_model);
    ENGINE_TRACE("");
    ENGINE_TRACE("Model: \"{}\"", model.path);
    ENGINE_TRACE("Skeleton: ");
    ENGINE_TRACE("  Bone map size (number of bones): {}", model.skeleton.boneMap.size());
    if(model.skeleton.boneMap.size() <= 30)
        for(auto const &[name, id] : model.skeleton.boneMap)
            ENGINE_TRACE("    [\"{}\": {}]", name, id);

    ENGINE_TRACE("Animations: {}", model.animations.size());
    for(auto const &animation : model.animations)
    {
        ENGINE_TRACE("-----------------");
        ENGINE_TRACE("Animation: \"{}\"", animation.name);
        ENGINE_TRACE("  Duration: {} ticks, tps: {}", animation.durationTicks, animation.ticksPerSecond);
        ENGINE_TRACE("  Bones size: {}", animation.bones.size());
        if(animation.bones.size())
        {
            size_t minKfSize = ~0ull;
            size_t maxKfSize = 0;
            for(auto const &bone : animation.bones)
            {
                minKfSize = glm::min(minKfSize, bone.size());
                maxKfSize = glm::max(maxKfSize, bone.size());
            }
            ENGINE_TRACE("  Number of keyframes: [ min: {}, max: {} ]", minKfSize, maxKfSize);
        }
    }

    ENGINE_TRACE("Meshes: {}", model.meshes.size());
    for(auto const &mesh : model.meshes)
    {
        ENGINE_TRACE("-----------------");

        ENGINE_TRACE("Geometry:");
        ENGINE_TRACE("  Indices:   {}", mesh.primitives.indices.size());
        ENGINE_TRACE("  Positions: {}", mesh.primitives.positions.size());
        ENGINE_TRACE("  TexCoords: {}", mesh.primitives.texCoords.size());
        ENGINE_TRACE("  Normals:   {}", mesh.primitives.normals.size());
        ENGINE_TRACE("  Tangents:  {}", mesh.primitives.tangents.size());
        ENGINE_TRACE("  BoneIDs:   {}", mesh.primitives.boneIDs.size());
        ENGINE_TRACE("  Weights:   {}", mesh.primitives.weights.size());
        
        ENGINE_TRACE("Material:");
        ENGINE_TRACE("Textures:");
        ENGINE_TRACE("  Albedo:       {}", printTexture(mesh.material.textures.albedo, registry));
        ENGINE_TRACE("  Metallic:     {}", printTexture(mesh.material.textures.metallic, registry));
        ENGINE_TRACE("  Roughness:    {}", printTexture(mesh.material.textures.roughness, registry));
        ENGINE_TRACE("  Ambient:      {}", printTexture(mesh.material.textures.ambient, registry));
        ENGINE_TRACE("  Normal:       {}", printTexture(mesh.material.textures.normal, registry));
        ENGINE_TRACE("  Displacement: {}", printTexture(mesh.material.textures.displacement, registry));
        ENGINE_TRACE("  Alpha:        {}", printTexture(mesh.material.textures.alpha, registry));
        ENGINE_TRACE("Properties:");
        ENGINE_TRACE("  Ambient:       {}", fmt::streamed(mesh.material.properties.ambient));
        ENGINE_TRACE("  Albedo:        {}", fmt::streamed(mesh.material.properties.albedo));
        ENGINE_TRACE("  Specular:      {}", fmt::streamed(mesh.material.properties.specular));
        ENGINE_TRACE("  Transmittance: {}", fmt::streamed(mesh.material.properties.transmittance));
        ENGINE_TRACE("  Emission:      {}", fmt::streamed(mesh.material.properties.emission));
        ENGINE_TRACE("  Shininess:     {}", mesh.material.properties.shininess);
        ENGINE_TRACE("  Metallic:      {}", mesh.material.properties.metallic);
        ENGINE_TRACE("  IOR:           {}", mesh.material.properties.ior);
    }
}

int main(int argc, char **argv) {
    constexpr unsigned toLoad = 6 + 1;
    float progress = 0;
    
    // std::thread loadingScreenThread{cooload::loadingScreen, nullptr}; // disable loading screen
    std::thread loadingScreenThread{cooload::loadingScreen, &progress};

    GLFWwindow* window;
    
    if (!glfwInit())
        return -1;
    progress += 1.0f / toLoad;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    window = glfwCreateWindow(800, 600, "engine", NULL, NULL);
    if (!window) {
        std::cout << "ERROR: failed to init the window!\n";
        return -1;
    }
    progress += 1.0f / toLoad;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    ecs::registry registry;

    auto e_window = registry.create(engine::Window{window});

    engine::input::setup(registry);
    engine::physics::setup(registry);
    engine::Logger::init();
    
    engine::Loader loader{registry};
    auto cube =    loader.load(engine::DataType::MODEL, "res/models/cube.obj");    progress += 1.0f / toLoad;
    auto suzanne = loader.load(engine::DataType::MODEL, "res/models/suzanne.obj"); progress += 1.0f / toLoad;
    auto fox =     loader.load(engine::DataType::MODEL, "res/models/fox.glb");     progress += 1.0f / toLoad;
    auto sponza =  loader.load(engine::DataType::MODEL, "res/models/sponza.glb");  progress += 1.0f / toLoad;

    registry.create(engine::Draw{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {1, 1, 2}
            ),
            45.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::Draw{suzanne}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {-1, 1, -5}
            ),
            -26.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::Draw{fox}, engine::ModelMatrix{
        glm::translate(
            glm::mat4{1.0f},
            {0, 0, 6}
        )
    });
    registry.create(engine::Draw{sponza}, engine::ModelMatrix{
        glm::translate(
            glm::mat4{1.0f},
            {0, 0, 0}
        )
    });
    
    ecs::entity e_camera = controller::createCamera(registry, e_window);
    auto &camera = registry.get<controller::ControllableCamera>(e_camera);
    camera.speed = 4;
    camera.sensitivity = 0.125;

    engine::EngineRenderer renderer1{};

    engine::EngineRenderer renderer{{
        .e_camera = e_camera
    }};
    renderer.setup(registry);
    renderer.processData(registry);

    printModelData(sponza, registry);

    progress = 1;
    loadingScreenThread.join();

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();

        // for(auto e : registry.view<engine::Orientation>())
        // {
        //     auto &c = registry.get<engine::Orientation>(e);
        // }

        engine::input::update(registry);
        controller::update(registry);
        engine::physics::update(registry, deltatime);
        renderer.draw(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
    }

    ENGINE_INFO("Exiting...");

    glfwTerminate();
}
