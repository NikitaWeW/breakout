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
        void deallocate() noexcept;
        
        public:
        ShaderProgram() noexcept = default;
        ShaderProgram(std::string const &directory, bool showLog = false);
        ~ShaderProgram();
        bool collectShaders(std::string const &directory) noexcept;
        bool compileShaders() noexcept;
        int getUniform(std::string const &name) const noexcept;
        void bind(unsigned slot = 0) const noexcept;

        inline std::vector<Shader> const &getShaders() const noexcept { return m_shaders; }
        inline std::vector<Shader> &getShaders() noexcept { return m_shaders; }
        inline std::string const &getPath() const noexcept { return m_dirPath; }
        inline std::string &getPath() noexcept { return m_dirPath; }
        inline std::string const &getLog() const noexcept { return m_log; }
    };
} // namespace opengl
