#include "engine/Renderer/EngineRenderer.hpp"
#include "engine/Logging/logging.hpp"
#include "detail.hpp"

namespace ogl = engine::renderer::ogl;

static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar *message, const void *) {
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

    if(severity == GL_DEBUG_SEVERITY_HIGH)
    {
        ENGINE_CORE_ERROR("{}: opengl {} severity {}, raised from {}: \n\t {}", error.id, error.severity, error.type, error.source, error.message);
        ENGINE_ASSERT_MSG(false, "high severity error in the opengl renderer!");
    }
    else
    {
        ENGINE_CORE_WARN("{}: opengl {} severity {}, raised from {}: \n\t {}", error.id, error.severity, error.type, error.source, error.message);
    }
}
static void setupOpengl(Registry &)
{
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
void engine::EngineRenderer::setupPipeline(Registry &reg)
{
    using namespace engine;
    renderer::RendererData &data = reg.get<renderer::RendererData>(reg.view<renderer::RendererData>().at(0));
    
    glCreateFramebuffers(1, &data.oitFBO.id);
    {
        std::array<GLenum, 3> const drawbuffers = { GL_NONE, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glNamedFramebufferDrawBuffers(data.oitFBO.id, drawbuffers.size(), drawbuffers.data());
    }

    glCreateFramebuffers(1, &data.mainFBO.id);

    glCreateFramebuffers(1, &data.SM.atlas.fbo.id);
    {
        std::array<GLenum, 1> const drawbuffers = { GL_NONE };
        glNamedFramebufferDrawBuffers(data.SM.atlas.fbo.id, drawbuffers.size(), drawbuffers.data());
    }

    data.shaders.screenShader                  = ogl::compileShader("shaders/hdrImage");
    data.shaders.propShader                    = ogl::compileShader("shaders/prop");
    data.shaders.oitCompositeShader            = ogl::compileShader("shaders/oitComposite");
    data.shaders.skyboxShader                  = ogl::compileShader("shaders/skybox");
    data.shaders.depthMapShader                = ogl::compileShader("shaders/depthMapOpaque");

    data.defaultTexture = ogl::makeTexture(engine::Bitmap<float>{1, 1, 3, std::array<float, 1*1*3>{
        1, 1, 1
    }.data()}, false);

    glCreateBuffers(1, &data.pointLightsSSBO.id);
    glCreateBuffers(1, &data.dirLightsSSBO.id);
    glCreateBuffers(1, &data.spotLightsSSBO.id);
    glCreateBuffers(1, &data.SM.lightsSSBO.id);
}

void engine::EngineRenderer::setup(Registry &reg)
{
    auto view = reg.view<renderer::RendererData>();
    if(!view.empty())
    {
        ENGINE_ASSERT_MSG(view.size() <= 1, "multiple instances of EngineRenderer in registry!");
        for(auto e : view)
            reg.destroy(e);
    }
    reg.create(renderer::RendererData{
        .context = context(),
    });
    setupOpengl(reg);
    setupPipeline(reg);
}
