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
#include "engine/animation/animation.hpp"
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
    ENGINE_INFO("");
    ENGINE_INFO("Model: \"{}\"", model.path);
    ENGINE_INFO("Skeleton: ");
    ENGINE_INFO("  Bone map size / number of bones: {}", model.skeleton.boneMap.size());
    if(model.skeleton.boneMap.size() <= 30)
        for(auto const &[name, id] : model.skeleton.boneMap)
            ENGINE_INFO("    [\"{}\": {}]", name, id);

    ENGINE_INFO("Animations: {}", model.animations.size());
    for(auto const &animation : model.animations)
    {
        ENGINE_INFO("-----------------");
        ENGINE_INFO("Animation: \"{}\"", animation.name);
        ENGINE_INFO("  Duration: {} ticks, tps: {}", animation.durationTicks, animation.ticksPerSecond);
        ENGINE_INFO("  Bones size: {}", animation.bones.size());
    }

    ENGINE_INFO("Meshes: {}", model.meshes.size());
    for(auto const &mesh : model.meshes)
    {
        ENGINE_INFO("-----------------");

        ENGINE_INFO("Geometry:");
        ENGINE_INFO("  Indices:   {}", mesh.geometry.indices.size());
        ENGINE_INFO("  Positions: {}", mesh.geometry.positions.size());
        ENGINE_INFO("  TexCoords: {}", mesh.geometry.texCoords.size());
        ENGINE_INFO("  Normals:   {}", mesh.geometry.normals.size());
        ENGINE_INFO("  Tangents:  {}", mesh.geometry.tangents.size());
        ENGINE_INFO("  BoneIDs:   {}", mesh.geometry.boneIDs.size());
        ENGINE_INFO("  Weights:   {}", mesh.geometry.weights.size());
        
        ENGINE_INFO("Material:");
        ENGINE_INFO("Textures:");
        ENGINE_INFO("  Albedo:       {}", printTexture(mesh.material.textures.albedo, registry));
        ENGINE_INFO("  Metallic:     {}", printTexture(mesh.material.textures.metallic, registry));
        ENGINE_INFO("  Roughness:    {}", printTexture(mesh.material.textures.roughness, registry));
        ENGINE_INFO("  Ambient:      {}", printTexture(mesh.material.textures.ambient, registry));
        ENGINE_INFO("  Normal:       {}", printTexture(mesh.material.textures.normal, registry));
        ENGINE_INFO("  Displacement: {}", printTexture(mesh.material.textures.displacement, registry));
        ENGINE_INFO("  Alpha:        {}", printTexture(mesh.material.textures.alpha, registry));
        ENGINE_INFO("Properties:");
        ENGINE_INFO("  Ambient:       {}", fmt::streamed(mesh.material.properties.ambient));
        ENGINE_INFO("  Albedo:        {}", fmt::streamed(mesh.material.properties.albedo));
        ENGINE_INFO("  Specular:      {}", fmt::streamed(mesh.material.properties.specular));
        ENGINE_INFO("  Emission:      {}", fmt::streamed(mesh.material.properties.emission));
        ENGINE_INFO("  Shininess:     {}", mesh.material.properties.shininess);
        ENGINE_INFO("  Metallic:      {}", mesh.material.properties.metallic);
        ENGINE_INFO("  IOR:           {}", mesh.material.properties.ior);
    }
}

int main(int argc, char **argv) {
    constexpr unsigned toLoad = 3 + 1;
    float progress = 0; // will be easier once i'll implement some kind of asset manager.
    
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
    auto cube =    loader.load(engine::DataType::MODEL, "res/models/cube.obj");
    auto suzanne = loader.load(engine::DataType::MODEL, "res/models/suzanne.obj");

    registry.create(engine::Instance{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {1, 1, -2}
            ),
            45.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::Instance{suzanne}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {-1, 1, 5}
            ),
            -26.0f,
            {1.0f, 2, -4}
        )
    });
    auto fox_instance = registry.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/fox.glb")
        }, 
        engine::ModelMatrix{
            glm::translate(
                glm::mat4{1.0f},
                {4, 0, -7}
            ) 
            * glm::rotate(
                glm::mat4{1.0f},
                glm::radians(-90.0f),
                {1, 0, 0}
            )
            * glm::scale(
                glm::mat4{1.0f},
                glm::vec3{0.01}
            )
        },
        engine::CurrentAnimation{
            .name = "Survey"
        }
    ); 
    registry.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/vampire/dancing_vampire.dae")
        }, 
        engine::ModelMatrix{
            glm::translate(
                glm::mat4{1.0f},
                {-3, 0, -7}
            ) 
            * glm::scale(
                glm::mat4{1.0f},
                glm::vec3{0.01}
            )
        }
        ,engine::CurrentAnimation{
            .name = "Hips"
        }
    );
    registry.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/Silly_Dancing.fbx")
        }, 
        engine::ModelMatrix{
            glm::translate(
                glm::mat4{1.0f},
                {0, 0, -7}
            ) 
            * glm::scale(
                glm::mat4{1.0f},
                glm::vec3{0.01}
            )
        }
        ,engine::CurrentAnimation{
            .name = "mixamo.com"
        }
    );
    registry.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/deccer_cubes/SM_Deccer_Cubes_Textured_Complex.glb")
        }, 
        engine::ModelMatrix{
            glm::translate(
                glm::mat4{1.0f},
                {-5, 0, 0}
            )
            // * glm::scale(
            //     glm::mat4{1.0f},
            //     glm::vec3{0.01}
            // )
        }
    );

    progress += 1.0f / toLoad;
    
    ecs::entity e_camera = controller::createCamera(registry, e_window);
    auto &camera = registry.get<controller::ControllableCamera>(e_camera);
    camera.speed = 4;
    camera.sensitivity = 0.125;

    engine::EngineRenderer renderer1{}; // check if everything initializes without conflicts..

    engine::EngineRenderer renderer{{
        .e_camera = e_camera
    }};
    renderer.setup(registry);
    renderer.processData(registry);

    engine::Animator animator;

    progress = 1;
    loadingScreenThread.join();

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();

        if(glm::mod<float>(glfwGetTime(), 5) < 0.01 && !registry.has<engine::AnimationTransition>(fox_instance))
        {
            auto const &model = registry.get<engine::Model>(registry.get<engine::Instance>(fox_instance).e_model);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, model.animations.size() - 1);
            
            std::string newAnimation = model.animations.at(distrib(gen)).name;
            auto const &newAnim = *std::find_if(model.animations.begin(), model.animations.end(), [&](engine::Animation const &animation){ return animation.name == newAnimation; });
            float duration = 0.5 * newAnim.durationTicks / newAnim.ticksPerSecond;

            // ENGINE_INFO("transition from {} to {} in {}s", current.name, newAnimation, duration);

            registry.emplace<engine::AnimationTransition>(fox_instance, engine::AnimationTransition{
                .to = newAnimation,
                .factorPerSecond = 1 / duration,
                .easeFunction = engine::ease::inOutCubic,
            });
        }

        engine::input::update(registry);
        controller::update(registry);
        animator.update(registry, deltatime);
        engine::physics::update(registry, deltatime);
        renderer.draw(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
        glfwSetWindowTitle(window, (std::to_string(deltatime * 1e3) + "ms | " + std::to_string(1/deltatime) + "fps").c_str());
    }

    ENGINE_INFO("Exiting...");

    glfwTerminate();
}
