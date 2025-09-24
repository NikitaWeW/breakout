#include <iostream>
#include <cmath>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/renderer/renderer.hpp"
#include "engine/loader/loader.hpp"
#include "engine/controller/controller.hpp"
#include "engine/physics/physics.hpp"
#include "engine/input/input.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/io.hpp"
#include "glm/gtc/noise.hpp"
#include "cooload.hpp"
#include <thread>
#include <random>
#include <stack>

class Deallocator {
public: 
    inline ~Deallocator() {
        glfwTerminate();
    }
};

void loadingScreen(float const *progress)
{
    cooload::SpinningCube cube = {};

    cooload::Bar progressBar;

    auto start = std::chrono::steady_clock::now();
    std::cout << "\x1B[?25l";
    do
    {
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(start - std::chrono::steady_clock::now()).count() * 1e-9f;
        
        auto prevSize = cube.imageSize;
        cube.imageSize = cooload::getConsoleSize();
        if(prevSize != cube.imageSize)
            cooload::resizeCube(cube);

        cube.rotationAxis = {
            glm::sin(time * 1.7f),
            glm::sin(time * 1.5f + 1.0f) * glm::cos(time * 0.3f),
            glm::cos(time * 1.2f)
        };

        cube.viewport.position = {0, 0};
        cube.viewport.size = glm::uvec2{static_cast<unsigned>(0.5 * glm::min(cube.imageSize.x, static_cast<unsigned>(cube.imageSize.y / cube.cellAspect)))};
        cube.viewport.size.y *= cube.cellAspect;

        auto cubeWidth = cube.viewport.position.x + cube.viewport.size.x + 1;

        progressBar.percentage = *progress;
        progressBar.begin = "Loading [";
        progressBar.end = "]";
        progressBar.width = glm::max(static_cast<int>(cube.imageSize.x - cubeWidth - 4), 0);

        cooload::gotoxy(cube.viewport.position);
        cooload::animateCube(cube, time);
        cooload::draw(cube);
        std::cout << cube.buffer.data();
        std::cout.flush();
        
        cooload::gotoxy({cubeWidth, cube.imageSize.y * 0.1f + 0});
        cooload::draw(progressBar);
        std::cout << progressBar.buffer.str();

        if(progressBar.percentage >= 1)
        {
            cooload::gotoxy({cubeWidth, cube.imageSize.y * 0.1f + 1});
            std::cout << "Done!";
        }
        
        cooload::gotoxy({0, cube.viewport.position.y + cube.viewport.size.y});
        std::cout.flush();
    }
    while(progressBar.percentage < 1);
    std::cout << "\x1B[?25h";
}

int main(int argc, char **argv) {
    constexpr unsigned toLoad = 7;
    float progress = 0;
    
    std::thread loadingScreenThread{loadingScreen, &progress};

    std::unique_ptr<Deallocator> cleanup{new Deallocator};
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

    progress += 1.0f / toLoad;
    engine::loader::setup(registry);

    progress += 1.0f / toLoad;
    engine::renderer::setup(registry);

    progress += 1.0f / toLoad;
    engine::input::setup(registry);
    progress += 1.0f / toLoad;
    engine::physics::setup(registry);
    
    auto cube = engine::loader::load(registry, "res/models/cube.obj");
    auto suzanne = engine::loader::load(registry, "res/models/suzanne.obj");

    registry.create(engine::renderer::Draw{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {4, 1, -2}
            ),
            45.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::renderer::Draw{cube}, engine::ModelMatrix{
        glm::rotate(
            glm::translate(
                glm::mat4{1.0f},
                {-4, -1, 2}
            ),
            -26.0f,
            {1.0f, 2, -4}
        )
    });
    registry.create(engine::renderer::Draw{suzanne}, engine::ModelMatrix{
        glm::translate(
            glm::mat4{1.0f},
            {0, 2, -6}
        ),
    });
    
    engine::renderer::processData(registry);
    ecs::entity rendererConfig = registry.view<engine::renderer::RendererContext>().at(0);

    ecs::entity e_camera = engine::controller::createCamera(registry, e_window);
    auto &camera = registry.get<engine::controller::ControllableCamera>(e_camera);
    camera.speed = 4;
    camera.sensitivity = 0.25;

    auto &config = registry.get<engine::renderer::RendererContext>(rendererConfig);
    config.e_camera = e_camera;

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
        engine::controller::update(registry);
        engine::physics::update(registry, deltatime);
        engine::renderer::render(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
    }
}
