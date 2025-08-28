#pragma once

#include "Object.hpp"
#include "glad/gl.h"
#include <string>
#include <vector>
#include <map>

namespace ogl
{
    /**
     * @brief Encapsulates an OpenGL shader program and its stages.
     *
     * Manages loading, compiling, linking, and binding of GLSL shader stages
     * from a directory. Provides utility for uniform, uniform block, and
     * shader storage block lookups with caching.
     */
    class ShaderProgram : public Object {
    public:
        /**
         * @brief Represents a single GLSL shader stage.
         *
         * Holds the OpenGL shader handle, its type (vertex, fragment, etc.),
         * and the source code read from file.
         */
        struct Shader {
            unsigned    renderID = 0;  ///< OpenGL shader object handle
            GLenum      type;          ///< Shader type (e.g., GL_VERTEX_SHADER)
            std::string source;        ///< GLSL source code
        };

    private:
        mutable std::map<std::string, int> m_uniformLocationCache; ///< Cache for uniform locations
        std::vector<Shader>               m_shaders;               ///< Collected shader stages
        std::string                       m_log;                   ///< Compilation and link log
        std::string                       m_dirPath;               ///< Directory path of shader files

        /**
         * @brief Deletes the OpenGL program and its attached shaders.
         *
         * Called during destruction when no more references remain
         * and canDeallocate() returns true.
         */
        void deallocate() noexcept;

    public:
        /**
         * @brief Default constructor.
         *
         * Creates an uninitialized ShaderProgram without generating
         * an OpenGL program handle.
         */
        ShaderProgram() noexcept = default;

        /**
         * @brief Loads, compiles, and links shaders from a directory.
         *
         * Scans the specified directory for shader source files,
         * compiles and links them into a program, and optionally
         * prints the compilation/link log.
         *
         * @param directory Directory containing shader source files.
         * @param showLog   If true, outputs the compile/link log.
         */
        explicit ShaderProgram(std::string const &directory, bool showLog = true);

        /**
         * @brief Destructor.
         *
         * Deletes the OpenGL program and shader objects if generated
         * and if canDeallocate() returns true.
         */
        ~ShaderProgram();

        /**
         * @brief Collects shader source files from the given directory.
         *
         * Reads all recognizable GLSL files, sets their type based on
         * extension, and stores them in m_shaders.
         *
         * @param directory Path to the shader directory.
         * @return True on successful collection; false otherwise.
         */
        bool collectShaders(std::string const &directory) noexcept;

        /**
         * @brief Compiles and links all collected shaders.
         *
         * Compiles each Shader in m_shaders, attaches them to a program,
         * and links. Errors are recorded in m_log.
         *
         * @return True if compilation and linking succeed; false on error.
         */
        bool compileShaders() noexcept;

        /**
         * @brief Retrieves the location of a uniform variable.
         *
         * Uses an internal cache to minimize calls to glGetUniformLocation.
         *
         * @param name Name of the uniform.
         * @return The uniform location, or -1 if not found.
         */
        int getUniform(std::string const &name) const noexcept;

        /**
         * @brief Retrieves the index of a uniform block.
         *
         * Calls glGetUniformBlockIndex for the named block.
         *
         * @param name Block name in the shader.
         * @return Block index, or -1 if not found.
         */
        int getUniformBlock(std::string const &name) const noexcept;

        /**
         * @brief Retrieves the index of a shader storage block.
         *
         * Calls glGetProgramResourceIndex for the named block.
         *
         * @param name Block name in the shader.
         * @return Block index, or -1 if not found.
         */
        int getStorageBlock(std::string const &name) const noexcept;

        /**
         * @brief Activates this shader program for rendering.
         *
         * Binds the program via glUseProgram(renderID).
         *
         * @param slot Ignored for shader programs; included for API consistency.
         */
        void bind(unsigned slot = 0) const noexcept override;

        /**
         * @brief Returns a const reference to the collected shaders.
         *
         * @return Vector of Shader structs.
         */
        inline std::vector<Shader> const &getShaders() const noexcept { return m_shaders; }

        /**
         * @brief Returns a mutable reference to the collected shaders.
         *
         * @return Vector of Shader structs.
         */
        inline std::vector<Shader> &getShaders() noexcept { return m_shaders; }

        /**
         * @brief Retrieves the directory path from which shaders were loaded.
         *
         * @return Directory path string.
         */
        inline std::string const &getPath() const noexcept { return m_dirPath; }

        /**
         * @brief Retrieves the directory path from which shaders were loaded.
         *
         * @return Directory path string.
         */
        inline std::string &getPath() noexcept { return m_dirPath; }

        /**
         * @brief Retrieves the compilation and link log.
         *
         * @return The log string containing warnings and errors.
         */
        inline std::string const &getLog() const noexcept { return m_log; }
    };
} // namespace ogl
