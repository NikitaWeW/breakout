#include <iostream>
#include <cmath>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "game/LevelParser.hpp"

#ifdef NDEBUG
extern constexpr bool DEBUG = false;
#else
extern constexpr bool DEBUG = true;
#endif

class Deallocator {
public: 
    inline ~Deallocator() {
        glfwTerminate();
    }
};
bool init(GLFWwindow** window) {
    assert(window);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    *window = glfwCreateWindow(640, 480, "breakout", NULL, NULL);
    if (!*window) {
        std::cout << "ERROR: failed to init the window!\n";
        return false;
    }

    glfwMakeContextCurrent(*window);
    glfwSwapInterval(DEBUG ? 0 : 1);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cout << "ERROR: Failed to initialize OpenGL context\n";
        return false;
    }

    return true;
}

namespace game
{
    void gameMain(GLFWwindow *mainWindow);
} // namespace game

int main(int argc, char **argv) {
    std::unique_ptr<Deallocator> cleanup{new Deallocator};
    GLFWwindow* window;
    if(!init(&window)) {
        std::cout << "failed to init!\n";
        return -1;
    };

    game::gameMain(window);
}
