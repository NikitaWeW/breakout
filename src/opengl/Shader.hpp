#pragma once
#include "Object.hpp"
#include "glad/gl.h"
#include <string>
#include <vector>
#include <map>

namespace opengl
{
    class ShaderProgram : public Object {
    public:
        struct Shader {
            unsigned renderID = 0;
            GLenum type;
            std::string source;
        };
    private:
        mutable std::map<std::string, int> m_UniformLocationCache;
        std::vector<Shader> m_shaders;
        std::string m_log;
        std::string m_dirPath;
        void deallocate();
    public:
        bool collectShaders(std::string const &directory);
        bool compileShaders();
        int getUniform(std::string const &name) const;
        void bind(unsigned slot = 0) const;

        inline std::vector<Shader> const &getShaders() const { return m_shaders; }
        inline std::vector<Shader> &getShaders() { return m_shaders; }
        inline std::string const &getPath() const { return m_dirPath; }
        inline std::string &getPath() { return m_dirPath; }
        inline std::string const &getLog() { return m_log; }
    };
} // namespace opengl
