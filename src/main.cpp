#include <iostream>
#include <memory>
#include <cassert>
#include <chrono>
#include <thread>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "opengl/Shader.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/IndexBuffer.hpp"
#include "utils/ControllableCamera.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/Texture.hpp"
#include "opengl/Framebuffer.hpp"
#include "utils/AABB.hpp"
#include "utils/Text.hpp"
#define basicLatin text::charRange(L'!', L'~')

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
}
class Deallocator {public: inline void operator()(void *) {
    glfwTerminate();
    this->~Deallocator(); // just in case
}};
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

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cout << "ERROR: Failed to initialize OpenGL context\n";
        return false;
    }
    std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << '.' << GLAD_VERSION_MINOR(version) << " (compatibility)\n";
    glDebugMessageCallback(debugCallback, nullptr);

    return true;
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
}
inline constexpr glm::vec3 hex(int hexValue) {
    return {
        ((hexValue >> 16) & 0xFF) / 255.0,
        ((hexValue >> 8) & 0xFF) / 255.0,
        ((hexValue) & 0xFF) / 255.0
    };
}

int main(int argc, char **argv) {
    std::unique_ptr<Deallocator, Deallocator> cleanup{new Deallocator};
    GLFWwindow* window;
    if(!init(&window)) {
        std::cout << "failed to init!\n";
        return -1;
    };
    Camera camera{{0, 0, -1}, {-90, 0, 0}, Camera::ProjectionType::ORTHO};
    camera.near = -1; camera.far = 1;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);

    float vertices[] = {
        -1,  1, 0,
         1,  1, 0,
         1, -1, 0,
        -1, -1, 0,

        0, 1,
        1, 1,
        1, 0,
        0, 0 
    };
    unsigned indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    opengl::VertexBuffer quadVBO{sizeof(vertices), vertices};
    opengl::VertexArray quadVAO{quadVBO, opengl::VertexBufferLayout{
        {3, GL_FLOAT, 0},
        {2, GL_FLOAT, 12 * sizeof(float)}
    }};
    opengl::IndexBuffer quadIBO{sizeof(indices), indices};
    opengl::ShaderProgram shader{"shaders/colorTexture"};
    opengl::Texture ballTexture{"res/textures/ball.png", true, true};

    text::Font font = text::Font("res/OpenSans-Light.ttf", basicLatin);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    double deltatime = 1;
    std::thread fpsShower{[&deltatime, &window](){while(!glfwWindowShouldClose(window)) {glfwSetWindowTitle(window, ("breakout -- " + std::to_string((int) glm::round(1 / deltatime)) + " FPS").c_str()); std::this_thread::sleep_for(std::chrono::milliseconds{500}); }}}; fpsShower.detach();
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        glfwGetWindowSize(window, &camera.width, &camera.height);
        camera.update(deltatime);
        glViewport(0, 0, camera.width, camera.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        font.drawText("text rendering still works.", {-0.9, 0.9}, 0.8, hex(0xFABC3C), camera.getProjectionMatrix());

        // quadVAO.bind();
        // quadIBO.bind();
        // glm::mat4 modelMat{1.0f};
        // font.getAtlas().texture.bind();
        // shader.bind();
        // glUniform1i(shader.getUniform("u_texture"), 0);
        // glUniform3f(shader.getUniform("u_color"), 1, 1, 1);
        // glUniformMatrix4fv(shader.getUniform("u_modelMat"), 1, GL_FALSE, &modelMat[0][0]);
        // glUniformMatrix4fv(shader.getUniform("u_viewMat"),      1, GL_FALSE, &camera.getViewMatrix()[0][0]);
        // glUniformMatrix4fv(shader.getUniform("u_projectionMat"),1, GL_FALSE, &glm::mat4(1.0f)[0][0]);
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
        deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1.0E-6;
    }
}

