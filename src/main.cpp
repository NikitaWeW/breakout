#include <iostream>
#include <memory>
#include <cassert>
#include <chrono>
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "opengl/Shader.hpp"
#include "opengl/VertexBuffer.hpp"
#include "opengl/IndexBuffer.hpp"
#include "utils/ControllableCamera.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/Texture.hpp"
#include "opengl/Framebuffer.hpp"
#include "ttf2mesh.h"

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

int main(int argc, char **argv) {
    Deallocator deallocator;
    std::unique_ptr<Deallocator, Deallocator> cleanup{&deallocator};
    // std::unique_ptr<void, Deallocator> cleanup{nullptr};
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
    opengl::VertexBuffer vbo{sizeof(vertices), vertices};
    opengl::VertexBufferLayout layout;
    layout.push({3, GL_FLOAT, 0}); // TODO: construct layouts from initializer lists
    layout.push({2, GL_FLOAT, 12 * sizeof(float)});
    opengl::VertexArray vao{vbo, layout};
    opengl::IndexBuffer ibo{sizeof(indices), indices};
    opengl::ShaderProgram shader{"shaders/colorTexture"};
    opengl::Texture ballTexture{"res/ball.png", true, true};

    constexpr unsigned atlasSize = 2048;
    constexpr unsigned glyfCount = L'~' - L' '; // !
    opengl::Texture atlas{atlasSize, atlasSize, GL_RED};
    
    { // TODO: export atlas using stb image write
        unsigned stepSize = atlasSize / glm::ceil(glm::sqrt(glyfCount));
        glm::vec2 currentPos = {0, 0};
        opengl::ShaderProgram atlasShader{"shaders/fontAtlas"};
        opengl::Framebuffer atlasFBO;
        atlasFBO.bind();
        atlasFBO.attach(atlas, GL_COLOR_ATTACHMENT0);
        assert(atlasFBO.isComplete());
        glBindVertexArray(0);
        
        ttf_t *font;
        ttf_load_from_file("res/JetBrainsMono-Bold.ttf", &font, false);
        assert(font);
        for(uint16_t currentChar = L' '; currentChar <= L'~'; ++currentChar) {
            // get the glyph mesh
            int index = ttf_find_glyph(font, currentChar);
            if(index < 0) {
                std::cout << "WARNING: failed to find \'" << currentChar << "\' glyph!\n";
                continue;
            }
            ttf_mesh_t *mesh;
            if(ttf_glyph2mesh(&font->glyphs[index], &mesh, TTF_QUALITY_NORMAL, TTF_FEATURES_DFLT) != TTF_DONE) {
                std::cout << "WARNING: failed to convert glyph \'" << (char) currentChar << "\' to mesh!\n";
                continue;
            }
    
            // draw the mesh to the atlas
            atlasShader.bind();
            glViewport(currentPos.x, currentPos.y, stepSize, stepSize);
            opengl::VertexBuffer meshVBO{mesh->nvert * sizeof(float) * 2, mesh->vert};
            opengl::IndexBuffer meshIBO{mesh->nfaces * 3 * sizeof(int), mesh->faces};
            glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), reinterpret_cast<void const *>(0));
            glEnableVertexAttribArray(0);
            glDrawElements(GL_TRIANGLES, mesh->nfaces, GL_UNSIGNED_INT, nullptr);

            currentPos.x += stepSize;
            if(currentPos.x + stepSize > atlasSize) {
                currentPos.x = 0;
                currentPos.y += stepSize;
                assert(currentPos.y + stepSize < atlasSize);
            }
    
            ttf_free_mesh(mesh);
        }

        ttf_free(font);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    

    double deltatime = 0;
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        glfwGetWindowSize(window, &camera.width, &camera.height);
        camera.update(deltatime);
        glViewport(0, 0, camera.width, camera.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        vao.bind();
        ibo.bind();
        glm::mat4 modelMat{1.0f};
        modelMat = glm::scale(modelMat, glm::vec3{0.5});
        atlas.bind();
        shader.bind();
        // ballTexture.bind();
        glUniform1i(shader.getUniform("u_texture"), 0);
        glUniform3f(shader.getUniform("u_color"), 1, 1, 1);
        glUniformMatrix4fv(shader.getUniform("u_modelMat"), 1, GL_FALSE, &modelMat[0][0]);
        glUniformMatrix4fv(shader.getUniform("u_viewMat"),      1, GL_FALSE, &camera.getViewMatrix()[0][0]);
        glUniformMatrix4fv(shader.getUniform("u_projectionMat"),1, GL_FALSE, &camera.getProjectionMatrix()[0][0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
        deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1.0E-6;
    }
}

