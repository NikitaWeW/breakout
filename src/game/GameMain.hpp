#pragma once
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "opengl/Shader.hpp"

#define basicLatin text::charRange(L'!', L'~')
namespace game
{
    void gameMain(GLFWwindow *mainWindow);
} // namespace game
