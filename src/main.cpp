#include <iostream>
#include <cmath>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "utils/ECS.hpp"
#include "game/LevelParser.hpp"

#ifdef NDEBUG
extern constexpr bool DEBUG = false;
#else
extern constexpr bool DEBUG = true;
#endif

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_OTHER)) return; // handled by ShaderProgram class 
    struct OpenGlError {
        GLuint id;
        std::string source;
        std::string type;
        std::string severity;
        std::string message;
    };
    OpenGlError error;
    error.id = id;
    error.message = message;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        error.source = "api";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        error.source = "window system";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        error.source = "shader compiler";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        error.source = "third party";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        error.source = "application";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        error.source = "unknown";
        break;

        default:
        error.source = "unknown";
        break;
    }
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        error.type = "error";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        error.type = "deprecated behavior warning";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        error.type = "udefined behavior warning";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        error.type = "portability warning";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        error.type = "performance warning";
        break;

        case GL_DEBUG_TYPE_OTHER:
        error.type = "message";
        break;

        case GL_DEBUG_TYPE_MARKER:
        error.type = "marker message";
        break;

        default:
        error.type = "unknown message";
        break;
    }
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        error.severity = "high";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        error.severity = "medium";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        error.severity = "low";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        error.severity = "notification";
        break;

        default:
        error.severity = "unknown";
        break;
    }

    std::cout << error.id << ": opengl " << error.severity << " severity " << error.type << ", raised from " << error.source << ":\n\t" << error.message << '\n';
    assert(severity != GL_DEBUG_SEVERITY_HIGH);
}
class Deallocator {
public: 
    inline ~Deallocator() {
        delete &ecs::getEntityManager(); // needed to explicitly deallocate opengl entities such as textures before context termination
        delete &ecs::getComponentManager();
        delete &ecs::getSystemManager();
        delete &game::getLevelParser();
        glfwTerminate();
    }
};
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
}
bool init(GLFWwindow** window) {
    assert(window);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
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
    glfwSetKeyCallback(*window, key_callback);
    glfwSetScrollCallback(*window, scroll_callback);
    glfwSwapInterval(DEBUG ? 0 : 1);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cout << "ERROR: Failed to initialize OpenGL context\n";
        return false;
    }
    glDebugMessageCallback(debugCallback, nullptr);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);  

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
