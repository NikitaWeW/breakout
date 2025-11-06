#include "ogl.hpp"
#include "engine/core/logging.hpp"
#include <filesystem>
#include <fstream>

namespace ogl = engine::renderer::ogl;

static bool compileProgramShader(ogl::Program::Shader &shader) noexcept {
    shader.id = glCreateShader(shader.type);
    char *source = &*shader.source.begin();
    glShaderSource(shader.id, 1, &source, nullptr);
    glCompileShader(shader.id);
    int success;
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &success);
    if(!success) {
        GLint log_size;
        glGetShaderiv(shader.id, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size > 0) {
            std::string log;
            log.resize(log_size);
            glGetShaderInfoLog(shader.id, log_size, nullptr, &log[0]);
            ENGINE_CORE_ERROR(log);
        }
        return false;
    }
    return true;
}

static bool linkProgram(ogl::Program &program) noexcept {
    program.id = glCreateProgram();
    for(auto const &shader : program.shaders) {
        glAttachShader(program.id, shader.id);
    }
    glLinkProgram(program.id);

    int success;
    glGetProgramiv(program.id, GL_LINK_STATUS, &success);
    if(!success) {
        GLint log_size;
        glGetProgramiv(program.id, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size > 0) {
            std::string log;
            log.resize(log_size);
            glGetProgramInfoLog(program.id, log_size, nullptr, &log[0]);
            ENGINE_CORE_ERROR(log);
        }
        return false;
    }
    return true;
}
static std::string_view getShaderTypeString(GLenum type) noexcept {
    switch (type)
    {
    case GL_VERTEX_SHADER:   return "vertex";
    case GL_GEOMETRY_SHADER: return "geometry";
    case GL_FRAGMENT_SHADER: return "fragment";
    case GL_COMPUTE_SHADER:  return "compute";
    default:                 return "unknown type";
    }
}
static std::string_view getFramebufferStatusString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:                       return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_UNDEFINED:                      return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED:                    return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default:                                            return "Unknown Framebuffer Status";
    }
}
static ogl::Program collectShaders(std::string_view dirpath)
{
    ogl::Program program;
    ENGINE_ASSERT_MSG(std::filesystem::exists(dirpath), "");
    program.dirpath = dirpath;
    for(auto const &directoryEntry : std::filesystem::recursive_directory_iterator{dirpath}) {
        if(!std::filesystem::is_regular_file(directoryEntry.path())) continue; 
        ogl::Program::Shader shader;

        std::string extension = directoryEntry.path().string().substr(directoryEntry.path().string().find_last_of('.'), directoryEntry.path().string().size());
        if(extension == ".vert") shader.type = GL_VERTEX_SHADER;
        else if(extension == ".geom") shader.type = GL_GEOMETRY_SHADER;
        else if(extension == ".frag") shader.type = GL_FRAGMENT_SHADER;
        else if(extension == ".comp") shader.type = GL_COMPUTE_SHADER;
        else {
            continue;
        }

        std::ifstream filestream{directoryEntry.path()};
        shader.source = std::string{std::istreambuf_iterator<char>{filestream}, std::istreambuf_iterator<char>{}};
        program.shaders.emplace_back(std::move(shader));
    }

    return program;
}


std::size_t ogl::getSizeOfGLType(GLenum type)
{
    switch (type) {
        case GL_BYTE:            return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:   return sizeof(GLubyte);
        case GL_SHORT:           return sizeof(GLshort);
        case GL_UNSIGNED_SHORT:  return sizeof(GLushort);
        case GL_INT:             return sizeof(GLint);
        case GL_UNSIGNED_INT:    return sizeof(GLuint);
        case GL_FLOAT:           return sizeof(GLfloat);
        case GL_DOUBLE:          return sizeof(GLdouble);
        default: 
            ENGINE_ASSERT_MSG(false, "unknown opengl type");
            return 0;
    }
}
ogl::Program ogl::compileShader(std::string_view dirpath)
{
    Program program = collectShaders(dirpath);

    for(Program::Shader &shader : program.shaders) {
        if(!compileProgramShader(shader)) {
            ENGINE_CORE_ERROR("failed to compile {} shader from \"{}\"!", getShaderTypeString(shader.type), dirpath);
            return Program{};
        }
    }

    if(!linkProgram(program)) {
        ENGINE_CORE_ERROR("failed to link program from \"{}\"!", dirpath);
        return Program{};
    }

    return program;
}

int ogl::getUniform(Program const &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetUniformLocation(program.id, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        ENGINE_CORE_WARN("uniform \"{}\" is not used or does not exist in shaders from \"{}\".", name, program.dirpath);
    }
    return location;
}

int ogl::getUniformBlock(Program const &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetUniformBlockIndex(program.id, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        ENGINE_CORE_WARN("uniform \"{}\" is not used or does not exist in shaders from \"{}\".", name, program.dirpath);
    }
    return location;
}

int ogl::getStorageBlock(Program const &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetProgramResourceIndex(program.id, GL_SHADER_STORAGE_BLOCK, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        ENGINE_CORE_WARN("uniform \"{}\" is not used or does not exist in shaders from \"{}\".", name, program.dirpath);
    }
    return location;
}

bool ogl::isComplete(Framebuffer const &fbo)
{
    unsigned status = glCheckNamedFramebufferStatus(fbo.id, GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        ENGINE_CORE_TRACE("non complete framebuffer checked: {}", getFramebufferStatusString(status));
    return status == GL_FRAMEBUFFER_COMPLETE;
}

ogl::Texture ogl::makeTexture(Bitmap<float> const &data, bool srgb)
{
    Texture texture;
    texture.width = data.getWidth();
    texture.height = data.getHeight();

    glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);

    // texture.handle = glGetTextureHandleARB(texture.id);
    // glMakeTextureHandleResidentARB(texture.handle);

    GLenum internalFormat = 0;
    GLenum format = 0;
    
    if(data.getNumComponents() == 3)
    {
        if(srgb)
            internalFormat = GL_SRGB;
        else
            internalFormat = GL_RGB16F;

        format = GL_RGB;
    }
    else if(data.getNumComponents() == 4)
    {
        if(srgb)
            internalFormat = GL_SRGB_ALPHA;
        else
            internalFormat = GL_RGBA16F;

        format = GL_RGBA;
    }
    else
    {
        ENGINE_ASSERT_MSG(false, "invalid number of texture channels");
    }

    bool small = data.getWidth() * data.getHeight() < 10000;

    glTextureStorage2D(texture.id, small ? 1 : 2, internalFormat, data.getWidth(), data.getHeight());
    glTextureSubImage2D(texture.id, 0, 0, 0, data.getWidth(), data.getHeight(), format, GL_FLOAT, data.getData());

    if(small) {
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glGenerateTextureMipmap(texture.id);
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texture;
}

ogl::Cubemap ogl::makeCubemap(std::array<Bitmap<float>, 6> const &data)
{
    ogl::Cubemap cubemap;
    cubemap.width = data[0].getWidth();
    cubemap.height = data[0].getHeight();
    cubemap.numSamples = 1;

    GLenum internalFormat = data[0].getNumComponents() == 4 ? GL_RGBA32F : GL_RGB32F;
    GLenum format = data[0].getNumComponents() == 4 ? GL_RGBA : GL_RGB;

    assert(cubemap.width == cubemap.height);
    unsigned faceSize = cubemap.width;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemap.id);
    glTextureStorage2D(
        cubemap.id,
        1,
        internalFormat,
        faceSize,
        faceSize
    );

    for(int i = 0; i < 6; ++i){
        glTextureSubImage3D(
            cubemap.id, 
            0,       // layer
            0, 0, i, // x,y,z
            cubemap.width, cubemap.height, // 2D image dimensions
            1,       // depth
            format,  // format
            GL_FLOAT,// data type
            data[i].getData()
        );
        
    }

    return cubemap;
}
