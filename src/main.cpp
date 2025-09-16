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

class Deallocator {
public: 
    inline ~Deallocator() {
        glfwTerminate();
    }
};

int main(int argc, char **argv) {
    std::unique_ptr<Deallocator> cleanup{new Deallocator};
    GLFWwindow* window;
    
    if (!glfwInit())
        return -1;

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

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    ecs::registry registry;

    auto e_window = registry.create(engine::Window{window});

    engine::loader::setup(registry);
    engine::renderer::setup(registry);

    auto cube = engine::loader::load(registry, "res/models/cube.obj");
    auto cube0 = registry.create(engine::renderer::Draw{cube});
    
    engine::renderer::processData(registry);
    ecs::entity rendererConfig = registry.view<engine::renderer::RendererContext>().at(0);

    ecs::entity e_camera = engine::controller::createCamera(registry);
    registry.get<engine::controller::ControllableCamera>(e_camera).window = e_window;

    auto &config = registry.get<engine::renderer::RendererContext>(rendererConfig);
    config.e_camera = e_camera;

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();

        engine::input::update(registry);
        engine::controller::update(registry);
        engine::physics::update(registry, deltatime);
        engine::renderer::render(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
    }
}
