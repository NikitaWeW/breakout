#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "opengl/Shader.hpp"
#include <filesystem>
#include "utils/Model.hpp"

#define basicLatin text::charRange(L'!', L'~')
namespace game
{
    struct Cache
    {
        std::map<std::filesystem::path, model::Model> modelCache;
        std::map<std::filesystem::path, opengl::Texture> textureCache;
    };
    extern Cache *globalCache;
    void gameMain(GLFWwindow *mainWindow);
} // namespace game

