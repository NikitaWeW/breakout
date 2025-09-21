#include "engine/config.hpp"
#include "ecs.hpp"
#include "glad/gl.h"
#include "../renderer.hpp"
#include <iostream>
#include "engine/renderer/renderer.hpp"
#include "renderer.hpp"
#include "render/render.hpp"

namespace engine::renderer::detail
{
    void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
        if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_OTHER)) return; // handled by ShaderProgram class 
        struct OpenGlError {
            GLuint id;
            std::string source;
            std::string type;
            std::string severity;
            std::string message;
            std::string level;
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
    
        ENGINE_OUT << error.id << ": opengl " << error.severity << " severity " << error.type << ", raised from " << error.source << ":\n\t" << error.message << '\n';
        ENGINE_ASSERT(severity != GL_DEBUG_SEVERITY_HIGH, "high severity error in the opengl renderer!");
    }
    
    void setupOpengl(ecs::registry &reg)
    {
        ENGINE_PROFILE()
        using namespace engine;
        auto windows = reg.view<engine::Window>();
        ENGINE_ASSERT(!windows.empty(), "no windows to render to!");
    
        gladLoadGL((GLADloadfunc) glfwGetProcAddress);
    
        ENGINE_ASSERT(version, "failed to load opengl!");
    
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(debugCallback, nullptr);
    }

    void setupPipeline(ecs::registry &reg)
    {
        detail::RendererData &data = reg.get<detail::RendererData>(reg.view<detail::RendererData>().at(0));
        
        glCreateFramebuffers(1, &data.oitFBO.id);
        {
            std::array<GLenum, 2> const drawbuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glNamedFramebufferDrawBuffers(data.oitFBO.id, drawbuffers.size(), drawbuffers.data());
        }

        glCreateFramebuffers(1, &data.mainFBO.id);

        data.plainColorShader = ogl::compileShader("src/engine/renderer/detail/shaders/" "plainColor");
    }
} // namespace engine::renderer::detail

void engine::renderer::detail::setup(ecs::registry &reg)
{
    ENGINE_PROFILE();
    detail::setupOpengl(reg);
    reg.create<RendererContext, detail::RendererData>();
    processData(reg);
    detail::setupPipeline(reg);
}
