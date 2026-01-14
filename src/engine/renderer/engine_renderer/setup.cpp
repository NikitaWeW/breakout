#include "engine/Header/Config.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "engine/Logging/Logging.hpp"
#include "detail.hpp"
#include "ogl.hpp"

using namespace engine;

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
static void setupOpengl(engine::Registry &)
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
void engine::EngineRenderer::recompileShaders()
{
    unwrap().compileShaders();
}
void engine::EngineRenderer::toggleDebugView()
{
    auto &debug = unwrap().mIsDebugView;
    debug = !debug;
    ENGINE_CORE_INFO("Debug view is now {}", debug);
}

void engine::EngineRendererImpl::compileShaders()
{
    ogl::recompileShader(mShaders.screenShader,       "shaders/hdrImage");
    ogl::recompileShader(mShaders.propShader,         "shaders/prop");
    ogl::recompileShader(mShaders.oitCompositeShader, "shaders/oitComposite");
    ogl::recompileShader(mShaders.skyboxShader,       "shaders/skybox");
    ogl::recompileShader(mShaders.depthMapShader,     "shaders/depthMapOpaque");

    if(!mShaders.valid())
        ENGINE_CORE_WARN("Failed to compile shaders, disabling the renderer until recompiled successfully!");
}
void engine::EngineRendererImpl::setupPipeline()
{
    using namespace engine;
    
    glCreateFramebuffers(1, &mOitFbo.id);
    {
        std::array<GLenum, 3> const drawbuffers = { GL_NONE, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glNamedFramebufferDrawBuffers(mOitFbo.id, drawbuffers.size(), drawbuffers.data());
    }

    glCreateFramebuffers(1, &mMainFbo.id);

    compileShaders();

    mDefaultTexture = ogl::makeTexture(engine::Bitmap<float>{2, 2, 3, std::array<float, 2*2*3>{
        0,0,0, 1,0,1, 1,0,1, 0,0,0,
    }.data()}, false);

}

struct EngineRendererAlreadySetupTag {};
void EngineRendererImpl::setup()
{
    ENGINE_ASSERT_MSG(mReg, "Invalid registry!");
    if(!mReg->view<EngineRendererAlreadySetupTag>().empty())
    {
        ENGINE_CORE_WARN("EngineRenderer already exists in this registry");
        return;
    }
    mReg->create<EngineRendererAlreadySetupTag>();
    setupOpengl(*mReg);
    mLightManager = renderer::LightManager{};
    mLightManager.setup();
    mLightManager.setCamera(Entity{*mReg, mConfig.e_camera});
    setupPipeline();
}

// FIXME: 
void EngineRenderer::setup() 
{
}
EngineRenderer::EngineRenderer(Registry &reg, EngineRendererConfig conf) : Handle(new EngineRendererImpl{})
{
    unwrap().mConfig = conf;
    unwrap().mReg = &reg;
    unwrap().setup();
}
EngineRenderer::~EngineRenderer()
{
    if(this->empty())
        return;

    auto tag = unwrap().mReg->view<EngineRendererAlreadySetupTag>();
    for(auto e : tag)
        unwrap().mReg->destroy(e);
}
