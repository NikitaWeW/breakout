#include <iostream>
#include <cmath>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/systems.hpp"
#include "engine/renderer/renderer.hpp"

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

    engine::SystemManager smanager;
    smanager.getRegistry().create<engine::renderer::Window>(engine::renderer::Window{ window });

    float deltatime = 0.1;

    while(!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        smanager.update(deltatime);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
