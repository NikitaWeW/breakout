#include "engine/renderer/engine_renderer/engineRenderer.hpp"
#include "engine/core/logging.hpp"
#include "detail.hpp"

namespace ogl = engine::renderer::ogl;

static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
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

    while(error.message.back() == '\n')
        error.message.pop_back();
    ENGINE_CORE_WARN("{}: opengl {} severity {}, raised from {}: \n\t {}", error.id, error.severity, error.type, error.source, error.message);
    ENGINE_ASSERT_MSG(severity != GL_DEBUG_SEVERITY_HIGH, "high severity error in the opengl renderer!");
}
static void setupOpengl(ecs::registry &reg)
{
    ENGINE_PROFILE();
    using namespace engine;

    gladLoadGL((GLADloadfunc) glfwGetProcAddress); // hope it works

    int numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    /*
    ENGINE_CORE_TRACE("Opengl extensions: {}", numExtensions);
    std::vector<std::string_view> extensions{numExtensions};
    for(int i = 0; i < numExtensions; ++i)
    {
        extensions[i] = reinterpret_cast<char const *>(glGetStringi(GL_EXTENSIONS, i));
    }
    */

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, nullptr);
}
void engine::EngineRenderer::setupPipeline(ecs::registry &reg)
{
    ENGINE_PROFILE();
    using namespace engine;
    renderer::RendererData &data = reg.get<renderer::RendererData>(reg.view<renderer::RendererData>().at(0));
    
    glCreateFramebuffers(1, &data.oitFBO.id);
    {
        std::array<GLenum, 3> const drawbuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glNamedFramebufferDrawBuffers(data.oitFBO.id, drawbuffers.size(), drawbuffers.data());
    }

    glCreateFramebuffers(1, &data.mainFBO.id);

    data.screenShader                  = ogl::compileShader("shaders/hdrImage");
    data.propShader                    = ogl::compileShader("shaders/prop");
    data.oitCompositeShader            = ogl::compileShader("shaders/oitComposite");
    data.skyboxShader                  = ogl::compileShader("shaders/skybox");
    data.depthMapShader                = ogl::compileShader("shaders/depthMapOpaque");
    data.depthMapOmnidirectionalShader = ogl::compileShader("shaders/depthMapOmnidirectionalOpaque");

    data.defaultTexture = ogl::makeTexture(engine::Bitmap<float>{1, 1, 3, std::array<float, 1*1*3>{
        1, 1, 1
    }.data()}, false);

    glCreateBuffers(1, &data.lightUBO.id);
    glNamedBufferData(data.lightUBO.id, sizeof(renderer::RendererData::LightsUBOStorage), nullptr, GL_DYNAMIC_DRAW);
}

engine::EngineRenderer::EngineRenderer(Context const &context)
{
    m_context = context;
}
void engine::EngineRenderer::setup(ecs::registry &reg)
{
    ENGINE_PROFILE();
    if(reg.view<renderer::RendererData>().empty())
        reg.create<renderer::RendererData>();
    setupOpengl(reg);
    setupPipeline(reg);
}
