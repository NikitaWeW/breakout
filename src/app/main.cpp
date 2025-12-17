#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>
#include <stack>

#include "scene.hpp"
#include "controller.hpp"
#include "cooload.hpp"

// The engine pipeline, game loop and other stuff will be abstracted 
// into the engine::Engine class one the engine becomes mature enough.

#include "engine/Engine.hpp"


int main(int argc, char **argv) {
    constexpr unsigned toLoad = 3 + 1;
    float progress = 0; // will be easier once i'll implement some kind of asset manager.
    
    // TODO: make loading screen work nicer with logging
    bool loadingScreen = false;

    std::thread loadingScreenThread{cooload::loadingScreen, loadingScreen ? &progress : nullptr};

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

    engine::Registry registry;
    registry.create(engine::Window{window});

    engine::Logger::init();

    createScene(registry);

    progress += 1.0f / toLoad;
    
    engine::input::setup(registry);

    ecs::entity e_camera = registry.view<Controller::ControllableCamera>().at(0);
    auto &camera = registry.get<Controller::ControllableCamera>(e_camera);
    camera.speed = 4;
    camera.sensitivity = 0.125;

    engine::EngineRenderer mainRenderer{};
    mainRenderer.context() = {
        .e_camera = e_camera,
        .MAX_SHADOW_MAP_FRAMES = 5
    };

    engine::IRenderer *renderer = &mainRenderer;
    renderer->setup(registry); 
    renderer->processData(registry);

    engine::Animator animator;
    Controller controller;

    engine::IPhysicsEngine *physics;
    engine::EnginePhysics mainPhysics;
    physics = &mainPhysics;

    progress = 1;
    loadingScreenThread.join();

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        glfwPollEvents();

        updateScene(registry, deltatime);

        engine::input::update(registry);
        controller.update(registry);
        animator.update(registry, deltatime);
        physics->update(registry, deltatime);
        renderer->draw(registry);
        
        glfwSwapBuffers(window);

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
        glfwSetWindowTitle(window, (std::to_string(deltatime * 1e3) + "ms | " + std::to_string(1/deltatime) + "fps").c_str());
        std::cout << "\033[s";
        ENGINE_INFO("dt: {:3f}ms;\t {:.3f} fps", deltatime * 1e3, 1.0f / deltatime);
        std::cout << "\033[u";
    }

    ENGINE_INFO("Exiting...");

    glfwTerminate();
}
