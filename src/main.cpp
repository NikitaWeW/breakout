#include <iostream>
#include <cmath>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/renderer/renderer.hpp"
#include "engine/loader/loader.hpp"

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

    float deltatime = 0.1;
    ecs::registry registry;

    engine::loader::setup(registry);
    auto cube = engine::loader::load(registry, "res/models/cube.obj");
    auto cube0 = registry.create(engine::renderer::Draw{cube});
    
    engine::renderer::setup(registry);
    engine::renderer::processData(registry);

    ecs::entity rendererConfig = registry.view<engine::renderer::RendererContext>().at(0);

    auto &config = registry.get<engine::renderer::RendererContext>(rendererConfig);
    config.camera.viewMat = glm::lookAt(glm::vec3{0, 2, 5}, glm::vec3{0, 0, 0}, glm::vec3{0,1,0});

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        engine::renderer::render(registry);

        glfwSwapBuffers(window);
        glfwPollEvents();

        deltatime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
    }
}
