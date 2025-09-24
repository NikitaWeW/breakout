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
#include "glm/gtc/noise.hpp"
#include "cooload.hpp"
#include <thread>
#include <random>
#include <stack>

int main(int argc, char **argv) {
    constexpr unsigned toLoad = 3;
    float progress = 0;
    
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

    engine::loader::setup(registry);
    engine::input::setup(registry);
    engine::physics::setup(registry);
    engine::Logger::init();
    
    auto cube = engine::loader::load(registry, "res/models/cube.obj");
    auto suzanne = engine::loader::load(registry, "res/models/suzanne.obj");

    registry.create(engine::Draw{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {4, 1, -2}
            ),
            45.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::Draw{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {-4, -1, 2}
            ),
            -26.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::Draw{suzanne}, engine::ModelMatrix{
        glm::translate(
            glm::mat4{1.0f},
            {0, 2, -6}
        ),
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

    float deltatime = 0.1;

    progress += 1.0f / toLoad;
    loadingScreenThread.join();

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

    glfwTerminate();
}
