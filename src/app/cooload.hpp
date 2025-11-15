#pragma once
/*
   .+------+ cooload -- a very small library to spice up console loading screens. *why is it even a library??*
 .' |    .'| Copyright (c) 2025 Nikita Martynau
+---+--+'  | https://opensource.org/license/mit
|   |  |   | thanks to https://github.com/tarantino07/cube.c for inspiration.
|  ,+--+---+ - rendering ascii cube to the terminal. 
|.'    | .'  - rendering ascii loading bars to the terminal.
+------+'    Spinning cube depends on glm for now.
[ ===============================================| ] 99%

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <vector>
#include <array>
#include <sstream>
#include "glm/glm.hpp"

namespace cooload
{
    /**
     * \brief The cube description
     */
    struct SpinningCube
    {
        glm::mat4 modelMat{1.0f};

        glm::mat4 projMat{1.0f};
        glm::vec3 rotationAxis = {1, 1, 1};
        
        std::vector<float> zbuffer;
        std::vector<char> buffer;

        glm::uvec2 imageSize = {0, 0};

        struct
        {
            glm::uvec2 position;
            glm::uvec2 size;
        } viewport;

        float distanceFromCam = 100;
        /** \brief The cube size. */
        float side = 20;
        /** \brief The face step size. */
        float increment = 0.6;
        /** \brief The aspect ratio of the cell (char). */
        float cellAspect = 0.5;
        char background = ' ';

        std::array<char, 6> faces = { '@', '$', '*', '#', '&', '8' };
    };

    /**
     * \brief The loading bar representation.
     */
    struct Bar
    {
        /** \brief The width of the bar */
        unsigned width = 10;
        /** \brief The normalized bar progress. */
        float percentage = 0;
        /** \brief The buffer containing the drawn bar. */
        std::stringstream buffer;
        /** \brief The head of the bar. */
        std::string begin = "[";
        /** \brief The tail of the bar. */
        std::string end   = "]";
        /** \brief The filled progress character. */
        char ch = '#';
        /** \brief The unfilled progress character. */
        char empty = ' ';
        /** \brief The last progress character. */
        char head = '#';
        /** \brief Indicates whether it should print the percentage after the bar. */
        bool printPercentage;
    };

    /**
     * \brief Resize the cube (buffers, projection, etc.)
     * \param cube The cube to process.
     * Should be called when the imageSize is changed.
     */
    void resizeCube(SpinningCube &cube);

    /**
     * \brief Rotate the cube.
     * \param cube The cube to process.
     * \param time The animation time.
     * Updates the cube model matrix.
     */
    void animateCube(SpinningCube &cube, float time = 0);

    /**
     * \brief Draws a cube to its buffer.
     * \param cube The cube to process.
     * Assumes the cube is resized, might segfault if not.
     */
    void draw(SpinningCube &cube);

    /**
     * \brief Gets the {columns, rows} console size.
     */
    glm::uvec2 getConsoleSize();

    /**
     * \brief Guess what it does.
     */
    void clearConsole();

    /**
     * \brief Change the console cursor position.
     */
    void gotoxy(glm::uvec2 pos);

    /**
     * \brief Draws a cube to its stringstream.
     * \param bar The bar to process.
     */
    void draw(Bar &bar);

    /**
     * \brief Some cool loading screen / example. 
     * Run in a separate thread.
     */
    void loadingScreen(float const *progress);
} // namespace cooload
