#include <iostream>
#include <cmath>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/engine.hpp"
#include "controller.hpp"
#include "boneDebugRenderer.hpp"
#include "cooload.hpp"
#include <thread>
#include <random>
#include <stack>
#include "scene.hpp"

// The engine pipeline, game loop and other stuff will be abstracted 
// into the engine::Engine class one the engine becomes mature enough.

int main(int argc, char **argv) {
    constexpr unsigned toLoad = 3 + 1;
    float progress = 0; // will be easier once i'll implement some kind of asset manager.
    
    std::thread loadingScreenThread{cooload::loadingScreen, nullptr}; // disable loading screen
    // std::thread loadingScreenThread{cooload::loadingScreen, &progress};

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

    registry.create(engine::Window{window});

    engine::Logger::init();
    
    createScene(registry);

    progress += 1.0f / toLoad;
    
    engine::input::setup(registry);

    ecs::entity e_camera = registry.view<Controller::ControllableCamera>().at(0);
    auto &camera = registry.get<Controller::ControllableCamera>(e_camera);
    camera.speed = 4;
    camera.sensitivity = 0.125;

    engine::EngineRenderer mainRenderer{{
        .e_camera = e_camera
    }};

    engine::IRenderer *renderer = &mainRenderer;
    renderer->setup(registry); 
    renderer->processData(registry);

    engine::Animator animator;
    Controller controller;

    engine::IPhysicsEngine *physics;
    engine::EnginePhysics mainPhysics;
    physics = &mainPhysics;

    engine::ModelMatrixAssembler modelMatrixAssembler;

    progress = 1;
    loadingScreenThread.join();

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();

        for(auto e_instance : registry.view<ChangeAnimationsTag, engine::Instance>())
        {
            if(glm::mod<float>(glfwGetTime(), 5) < 0.01 && !registry.has<engine::AnimationTransition>(e_instance))
            {
                auto const &model = registry.get<engine::Model>(registry.get<engine::Instance>(e_instance).e_model);
    
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(0, model.animations.size() - 1);
                
                std::string newAnimation = model.animations.at(distrib(gen)).name;
                auto const &newAnim = *std::find_if(model.animations.begin(), model.animations.end(), [&](engine::Animation const &animation){ return animation.name == newAnimation; });
                float duration = 0.5 * newAnim.durationTicks / newAnim.ticksPerSecond;
    
                // ENGINE_INFO("transition from {} to {} in {}s", current.name, newAnimation, duration);
    
                registry.emplace<engine::AnimationTransition>(e_instance, engine::AnimationTransition{
                    .to = newAnimation,
                    .factorPerSecond = 1 / duration,
                    .easeFunction = engine::ease::inOutCubic,
                });
            }
        }

        engine::input::update(registry);
        controller.update(registry);
        animator.update(registry, deltatime);
        physics->update(registry, deltatime);
        modelMatrixAssembler.update(registry);
        renderer->draw(registry);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
        glfwSetWindowTitle(window, (std::to_string(deltatime * 1e3) + "ms | " + std::to_string(1/deltatime) + "fps").c_str());
    }

    ENGINE_INFO("Exiting...");

    glfwTerminate();
}
