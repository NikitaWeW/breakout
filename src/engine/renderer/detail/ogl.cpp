#include "ogl.hpp"
#include <filesystem>
#include <fstream>

namespace ogl = engine::renderer::detail::ogl;

namespace engine::renderer::detail::ogl::detail // D:
{
    static bool compileShader(ogl::Program::Shader &shader) noexcept {
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
                ENGINE_OUT << log << '\n';
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
                ENGINE_OUT << log << '\n';
            }
            return false;
        }
        return true;
    }
    static std::string_view shaderTypeToString(GLenum type) noexcept {
        switch (type)
        {
        case GL_VERTEX_SHADER:   return "vertex";
        case GL_GEOMETRY_SHADER: return "geometry";
        case GL_FRAGMENT_SHADER: return "fragment";
        case GL_COMPUTE_SHADER:  return "compute";
        default:                 return "unknown type";
        }
    }
    static ogl::Program collectShaders(std::string_view dirpath)
    {
        ogl::Program program;
        ENGINE_ASSERT(std::filesystem::exists(dirpath), "");
        program.dirpath = dirpath;
        for(auto const &directoryEntry : std::filesystem::recursive_directory_iterator{dirpath}) {
            if(!std::filesystem::is_regular_file(directoryEntry.path())) continue; 
            Program::Shader shader;

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
} // namespace engine::renderer::detail::ogl::detail


ogl::Program ogl::compileShader(std::string_view dirpath)
{
    Program program = detail::collectShaders(dirpath);

    for(Program::Shader &shader : program.shaders) {
        if(!detail::compileShader(shader)) {
            ENGINE_OUT << "failed to compile " << detail::shaderTypeToString(shader.type) << " shader from \"" << dirpath << "\"\n";
            return Program{};
        }
    }

    if(!detail::linkProgram(program)) {
        ENGINE_OUT << "failed to link program \"" << dirpath << "\"\n";
        return Program{};
    }
}

int ogl::getUniform(ogl::Program &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetUniformLocation(program.id, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        std::cout << "uniform \"" << name << "\" is not used or does not exist in shaders from \"" << program.dirpath << "\".\n";
    }
    return location;
}

int ogl::getUniformBlock(ogl::Program &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetUniformBlockIndex(program.id, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        std::cout << "uniform \"" << name << "\" is not used or does not exist in shaders from \"" << program.dirpath << "\".\n";
    }
    return location;
}

int ogl::getStorageBlock(ogl::Program &program, std::string_view name)
{
    if(program.locationCache.find(name) != program.locationCache.end()) return program.locationCache[name];
    int location = glGetProgramResourceIndex(program.id, GL_SHADER_STORAGE_BLOCK, name.data());
    program.locationCache[name] = location;
    if(location == -1) {
        std::cout << "uniform \"" << name << "\" is not used or does not exist in shaders from \"" << program.dirpath << "\".\n";
    }
    return location;
}

bool ogl::isComplete(Framebuffer const &fbo)
{
    return glCheckNamedFramebufferStatus(fbo.id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

ogl::Texture ogl::makeTexture(Bitmap<float> data, bool srgb)
{
    ogl::Texture texture;
    texture.width = data.getWidth();
    texture.height = data.getHeight();

    glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);

    GLenum format = 0;
    if(srgb)
    {
        if(data.getNumComponents() == 3)
            format = GL_SRGB_ALPHA;
        else if(data.getNumComponents() == 4)
            format = GL_SRGB;
    } else {
        if(data.getNumComponents() == 3)
            format = GL_RGBA16F;
        else if(data.getNumComponents() == 4)
            format = GL_RGB16F;
    }

    glTextureSubImage2D(texture.id, 0, 0, 0, data.getWidth(), data.getHeight(), format, GL_FLOAT, data.getData());

    if(data.getWidth() * data.getHeight() > 10000) {
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glGenerateTextureMipmap(texture.id);
        glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texture;
}
