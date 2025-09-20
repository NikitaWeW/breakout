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

void loadingScreen(float const *progress, std::string *job)
{
    cooload::SpinningCube cube = {};

    cooload::Bar progressBar;

    std::cout << "\x1B[?25l";
    while(true)
    {
        if(*progress >= 1)
        {
            std::cout << "\nDone!\n";
            break;
        }
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() * 0.25e-8;
        
        auto prevSize = cube.imageSize;
        cube.imageSize = cooload::getConsoleSize();
        if(prevSize != cube.imageSize)
            cooload::resizeCube(cube);

        glm::vec3 baseAxis = {
            glm::sin(time * 0.7f) * glm::cos(time * 0.3f),
            glm::sin(time * 0.5f + 1.0f),
            glm::cos(time * 0.2f) * glm::sin(time * 0.9f)
        };
        float noise = glm::perlin(glm::vec3(time * 41.2f + 1941, time * 12.6f - 1522, time * 12.7f + 9814));
        glm::vec3 noiseAxis = {
            glm::sin(noise * 6.28f),
            glm::cos(noise * 4.71f),
            glm::sin(noise * 3.14f)
        };
        cube.rotationAxis = glm::normalize(baseAxis + 0.4f * noiseAxis);

        cube.viewport.position = {0, 0};
        cube.viewport.size = glm::uvec2{static_cast<unsigned>(0.5 * glm::min(cube.imageSize.x, static_cast<unsigned>(cube.imageSize.y / cube.cellAspect)))};
        cube.viewport.size.y *= cube.cellAspect;

        auto cubeWidth = cube.viewport.position.x + cube.viewport.size.x + 1;
        auto subProcessHeight = cube.imageSize.y * 0.9 - (cube.imageSize.y * 0.1 + 1);

        progressBar.percentage = *progress;
        progressBar.begin = "Loading [ ";
        progressBar.width = glm::max(static_cast<int>(cube.imageSize.x - cubeWidth - 4), 0);

        cooload::gotoxy(cube.viewport.position);
        cooload::animateCube(cube, time);
        cooload::draw(cube);
        std::cout << cube.buffer.data();
        std::cout.flush();
        
        cooload::gotoxy({cubeWidth, cube.imageSize.y * 0.1f + 0});
        cooload::draw(progressBar);
        std::cout << progressBar.buffer.str();
        
        cooload::gotoxy({cubeWidth, cube.imageSize.y * 0.1f + 1});
        std::cout << *job;

        cooload::gotoxy({0, cube.viewport.position.y + cube.viewport.size.y});
        std::cout.flush();
    }
    std::cout << "\x1B[?25h";
}


#define LOAD_JOB(name) progress += 1.0f / toLoad; currentJob = name

int main(int argc, char **argv) {
    unsigned toLoad = 6;
    float progress = 0;
    std::string currentJob = "";
    
    std::thread loadingScreenThread{loadingScreen, &progress, &currentJob};

    std::unique_ptr<Deallocator> cleanup{new Deallocator};
    GLFWwindow* window;
    
    currentJob = "Initializing GLFW";
    if (!glfwInit())
        return -1;
    LOAD_JOB("Initializing the window");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    window = glfwCreateWindow(640, 480, "engine", NULL, NULL);
    if (!window) {
        std::cout << "ERROR: failed to init the window!\n";
        return -1;
    }
    LOAD_JOB("Making GLFW context");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    ecs::registry registry;
    std::cout << "message\n";

    auto e_window = registry.create(engine::Window{window});

    LOAD_JOB("Setting up loader");
    std::cout << "warning\n";
    engine::loader::setup(registry);

    LOAD_JOB("Setting up renderer");
    engine::renderer::setup(registry);

    LOAD_JOB("Setting up Input");
    engine::input::setup(registry);
    
    LOAD_JOB("Setting up Input");
    engine::physics::setup(registry);
    
    auto cube = engine::loader::load(registry, "res/models/cube.obj");
    auto cube0 = registry.create(engine::renderer::Draw{cube});
    
    engine::renderer::processData(registry);
    ecs::entity rendererConfig = registry.view<engine::renderer::RendererContext>().at(0);

    ecs::entity e_camera = engine::controller::createCamera(registry);
    registry.get<engine::controller::ControllableCamera>(e_camera).window = e_window;

    auto &config = registry.get<engine::renderer::RendererContext>(rendererConfig);
    config.e_camera = e_camera;

    float deltatime = 0.1;

    loadingScreenThread.join();

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        cooload::clearConsole();
        ENGINE_OUT << deltatime << "s \t" << (1 / deltatime) << "Hz\n";

        for(auto e : registry.view<engine::Camera>())
        {
            auto &c = registry.get<engine::Camera>(e);
            ENGINE_OUT << c.viewMat << '\n';
        }

        engine::input::update(registry);
        engine::controller::update(registry);
        engine::physics::update(registry, deltatime);
        engine::renderer::render(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1e-9f;
    }
}
