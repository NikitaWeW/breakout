#include "Shader.hpp"
#include <fstream>
#include <cassert>
#include <filesystem>
#include <iostream>

bool compileShader(opengl::ShaderProgram::Shader &shader, std::string &log) {
    shader.renderID = glCreateShader(shader.type);
    char *source = &*shader.source.begin();
    glShaderSource(shader.renderID, 1, &source, nullptr);
    glCompileShader(shader.renderID);
    int success;
    glGetShaderiv(shader.renderID, GL_COMPILE_STATUS, &success);
    if(!success) {
        GLint log_size;
        glGetShaderiv(shader.renderID, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size > 0) {
            log.resize(log_size);
            glGetShaderInfoLog(shader.renderID, log_size, nullptr, &log[0]);
        }
        return false;
    }
    return true;
}

bool linkProgram(unsigned &program, std::vector<opengl::ShaderProgram::Shader> shaders, std::string &log) {
    program = glCreateProgram();
    for(auto const &shader : shaders) {
        glAttachShader(program, shader.renderID);
    }
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        GLint log_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
        if(log_size > 0) {
            log.resize(log_size);
            glGetProgramInfoLog(program, log_size, nullptr, &log[0]);
        }
        return false;
    }
    return true;
}

void opengl::ShaderProgram::deallocate()
{
    if(m_renderID) glDeleteProgram(m_renderID);
    for(Shader const &shader : m_shaders) {
        if(shader.renderID) glDeleteShader(shader.renderID);
    }   
}

bool opengl::ShaderProgram::collectShaders(std::string const &directory)
{
    assert(std::filesystem::exists(directory));
    m_log = "";
    for(auto const &directoryEntry : std::filesystem::recursive_directory_iterator{directory}) {
        if(!std::filesystem::is_regular_file(directoryEntry.path())) continue; 
        Shader shader;

        std::string extension = directoryEntry.path().string().substr(directoryEntry.path().string().find_last_of('.'), directoryEntry.path().string().size());
        if(extension == ".vert") shader.type = GL_VERTEX_SHADER;
        else if(extension == ".geom") shader.type = GL_GEOMETRY_SHADER;
        else if(extension == ".frag") shader.type = GL_FRAGMENT_SHADER;
        else if(extension == ".comp") shader.type = GL_COMPUTE_SHADER;
        else {
            m_log.append("unrecognised shader extension: \"" + directoryEntry.path().extension().string() + "\"\n");
            // return false;
            continue;
        }

        std::ifstream filestream{directoryEntry.path()};
        shader.source = std::string{std::istreambuf_iterator<char>{filestream}, std::istreambuf_iterator<char>{}};
        m_shaders.push_back(shader);
    }
    return true;
}

std::string shaderTypeToString(unsigned type) {
    switch (type)
    {
    case GL_VERTEX_SHADER:   return "vertex";
    case GL_GEOMETRY_SHADER: return "geometry";
    case GL_FRAGMENT_SHADER: return "fragment";
    case GL_COMPUTE_SHADER:  return "compute";
    default:                 return "unknown type";
    }
}
bool opengl::ShaderProgram::compileShaders()
{
    if(canDeallocate()) deallocate();

    m_UniformLocationCache.erase(m_UniformLocationCache.begin(), m_UniformLocationCache.end());
    m_log = "";
    
    for(Shader &shader : m_shaders) {
        if(!compileShader(shader, m_log)) {
            m_log.insert(0, "failed to compile " + shaderTypeToString(shader.type) + " shader\n");
            return false;
        }
    }

    if(!linkProgram(m_renderID, m_shaders, m_log)) {
        m_log.insert(0, "failed to link shader program\n");
        return false;
    }

    return true;
}

int opengl::ShaderProgram::getUniform(std::string const &name) const
{
    if(m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) return m_UniformLocationCache[name];
    int location = glGetUniformLocation(m_renderID, name.c_str());
    m_UniformLocationCache[name] = location;
    if(location == -1) {
        std::cout << "uniform \"" << name << "\" in shaders \"" << getPath() << "\" is not used or does not exist.\n";
    }
    return location;
}

void opengl::ShaderProgram::bind(unsigned slot) const { glUseProgram(m_renderID); }
