#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <thread>
#include "CLI/CLI.hpp"

#include "engine/DSA/Data.hpp"
#include "engine/Header/Config.hpp"
#include "engine/Input/Input.hpp"
#include "engine/Logging/Logging.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "scene.hpp"
#include "controller.hpp"
#include "cooload.hpp"

// The engine pipeline, game loop and other stuff will be abstracted 
// into the engine::Engine class one the engine becomes mature enough.

#include "engine/Engine.hpp"

#define INC_PROGRESS() progress += 1.0f / toLoad;

int main(int argc, char const **argv) {
    // TODO: make loading screen work nicer with logging
    bool vsync = true;
    bool loadingScreen = false;
    std::string scenePath = "";

    {
        CLI::App cli("An engine example");
        cli.add_flag("-l,--loading-screen", loadingScreen, "Enable loading screen");
        cli.add_option("--path", scenePath, "The path to the scene json file")->required();
    
        CLI11_PARSE(cli, argc, argv);
    }

    constexpr unsigned toLoad = 3 + 1;
    float progress = 0; // will be easier once i'll implement some kind of asset manager.

    std::thread loadingScreenThread{cooload::loadingScreen, loadingScreen ? &progress : nullptr};

    engine::Logger::init();
    if(!loadingScreen)
        ENGINE_INFO("Starting up...");

    GLFWwindow* window;
    
    if(!glfwInit())
        return -1;
    INC_PROGRESS();
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
    INC_PROGRESS();
    glfwMakeContextCurrent(window);
    glfwSwapInterval(vsync);


    engine::Registry reg;
    reg.create(engine::Window{window});
    auto e_listener = reg.create<engine::input::InputListener>();
    auto &listener = reg.get<engine::input::InputListener>(e_listener);

    SceneLoader loader{reg};
    ENGINE_INFO("Loading \"{}\" scene...", scenePath);
    auto scene = loader.load(scenePath);

    if(!scene.valid)
    {
        ENGINE_ERROR("Failed to load the scene \"{}\"", scenePath);
        progress = 1;
        loadingScreenThread.join();
        return -1;
    }

    INC_PROGRESS();
    
    engine::input::setup(reg);

    ENGINE_ASSERT(!reg.view<Controller::ControllableCamera>().empty());
    ecs::entity e_camera = reg.view<Controller::ControllableCamera>().at(0);
    auto mainRenderer = std::make_unique<engine::EngineRenderer>(reg, engine::EngineRendererConfig{
        .e_camera = e_camera,
        .MAX_SHADOW_MAP_FRAMES = 3
    });
    engine::IRenderer *renderer = mainRenderer.get();

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

        updateScene(reg, deltatime);

        for(; !listener.keyEvents.empty(); listener.keyEvents.pop())
        {
            auto const &event = listener.keyEvents.front();
            if(event.key == GLFW_KEY_R && event.action == GLFW_PRESS)
            {
                ENGINE_CORE_INFO("Recompiling shaders!");
                mainRenderer->recompileShaders();

                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                {
                    ENGINE_CORE_INFO("Reloading scene!");
                    for(auto e : scene.entities)
                        e.reg().destroy(e.entity());
                    scene.entities.clear();
                    scene = loader.load(scene.path);

                    ENGINE_ASSERT(!reg.view<Controller::ControllableCamera>().empty());
                    e_camera = reg.view<Controller::ControllableCamera>().at(0);
                    mainRenderer.reset();
                    mainRenderer = std::make_unique<engine::EngineRenderer>(reg, engine::EngineRendererConfig{
                        .e_camera = e_camera,
                        .MAX_SHADOW_MAP_FRAMES = 3
                    });
                    renderer = mainRenderer.get();
                }
            }
            if(event.key == GLFW_KEY_F && event.action == GLFW_PRESS)
                mainRenderer->toggleDebugView();
            if(event.key == GLFW_KEY_V && event.action == GLFW_PRESS)
            {
                vsync = !vsync;
                glfwSwapInterval(vsync);
                ENGINE_INFO("Toggled vsync to {}", vsync);
            }
            if(event.key == GLFW_KEY_C && event.action == GLFW_PRESS)
            {
                auto &transform = reg.get<engine::Transform>(e_camera);
                ENGINE_INFO("Camera: e{}, pos: {}, \tdir: {}", e_camera, fmt::streamed(transform.position), fmt::streamed(glm::vec3(0, 0, -1) * transform.orientation));
            }
        }

        // TODO: better windowing and input!!
        engine::input::update(reg);
        controller.update(reg);
        animator.update(reg, deltatime);
        physics->update(reg, deltatime);
        renderer->draw();

        glfwSwapBuffers(window);

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;

        // TODO: Better console management: write to string streams and draw the buffers to the terminal.
        glfwSetWindowTitle(window, (std::to_string(deltatime * 1e3) + "ms | " + std::to_string(1/deltatime) + "fps").c_str());
        // std::cout << "\033[s";
        // ENGINE_INFO("dt: {:3f}ms;\t {:.3f} fps", deltatime * 1e3, 1.0f / deltatime);
        // std::cout << "\033[u";
        // break;
    }

    ENGINE_INFO("Exiting...");

    glfwTerminate();
}
