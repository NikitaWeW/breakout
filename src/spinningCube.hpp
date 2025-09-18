/*
   .+------+ Spinning cube.
 .' |    .'| Copyright (c) 2025 Nikita Martynau
+---+--+'  | https://opensource.org/license/mit
|   |  |   | thanks to https://github.com/tarantino07/cube.c for inspiration.
|  ,+--+---+ A really small library for rendering ascii cube to the terminal. 
|.'    | .'  Might be useful for console loading screens.
+------+'    Depends on glm for now.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include "glm/glm.hpp"
#include <iostream>
#include <memory>

namespace cube
{
    /**
     * \brief The cube description
     */
    struct SpinningCube
    {
        /** \brief The cube matrix. */
        glm::mat4 modelMat{1.0f};

        /** \brief The projection matrix. */
        glm::mat4 projMat{1.0f};
        /** \brief The axis the cube is rotated around when calling animateCube. */
        glm::vec3 rotationAxis = {1, 1, 1};
        
        /** \brief Inner depth buffer. */
        std::vector<float> zbuffer;
        /** \brief The color buffer. */
        std::vector<char> buffer;

        /** \brief The size of the viewport. */
        glm::uvec2 imageSize = {160, 40};

        /** \brief The offset of the cube when calling animateCube. */
        float distanceFromCam = 100;
        /** \brief The cube size. */
        float side = 20;
        /** \brief The face step size. */
        float increment = 0.6;
        /** \brief The aspect ratio of the cell (char). */
        float cellAspect = 0.5;
        /** \brief The clear color char. */
        char background = ' ';
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
    void drawCube(SpinningCube const &cube);

    /**
     * \brief Prints the cube buffer to the given stream.
     * \param cube The cube to process.
     * \param stream The stream to output to.
     */
    void printCube(SpinningCube const &cube, std::ostream &stream = std::cout);

    /**
     * \brief Gets the {columns, rows} console size.
     */
    glm::uvec2 getConsoleSize();
} // namespace cube

// implementation
#include "glm/gtc/matrix_transform.hpp"

// for console size
#ifdef WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#endif
namespace cube
{
    inline void resizeCube(SpinningCube &cube)
    {
        cube.zbuffer.resize(cube.imageSize.x * cube.imageSize.y);
        cube.buffer.resize(cube.imageSize.x * cube.imageSize.y + 1);
        cube.projMat = glm::perspective<float>(45, cube.cellAspect * ((float) cube.imageSize.x / cube.imageSize.y), 0.01, 250);
    }

    inline void animateCube(SpinningCube &cube, float time)
    {
        cube.modelMat = glm::rotate(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -cube.distanceFromCam}), time, cube.rotationAxis);
    }

    inline void calculateForSurface(SpinningCube &cube, glm::vec3 pos, char ch)
    {
        glm::vec4 p = cube.projMat * cube.modelMat * glm::vec4{pos, 1};
        p /= p.w;
        p.x = (p.x * 0.5 + 0.5) * cube.imageSize.x;
        p.y = (p.y * 0.5 + 0.5) * cube.imageSize.y;
        
        size_t idx = static_cast<int>(p.x) + static_cast<int>(p.y) * cube.imageSize.x;
        if (idx >= 0 && idx < cube.imageSize.x * cube.imageSize.y) {
            if (p.z < cube.zbuffer[idx]) {
                cube.zbuffer[idx] = p.z;
                cube.buffer[idx] = ch;
            }
        }
    }

    inline void drawCube(SpinningCube &cube)
    {
        std::fill(cube.buffer.begin(), cube.buffer.end(), cube.background);
        std::fill(cube.zbuffer.begin(), cube.zbuffer.end(), 1.0f);

        for(float cubeX = -cube.side; cubeX < cube.side; cubeX += cube.increment)
        {
            for(float cubeY = -cube.side; cubeY < cube.side; cubeY += cube.increment)
            {
                calculateForSurface(cube, {cubeX, cubeY, -cube.side},  '@');
                calculateForSurface(cube, {cube.side, cubeY, cubeX},   '$');
                calculateForSurface(cube, {-cube.side, cubeY, -cubeX}, '*');
                calculateForSurface(cube, {-cubeX, cubeY, cube.side},  '#');
                calculateForSurface(cube, {cubeX, -cube.side, -cubeY}, '&');
                calculateForSurface(cube, {cubeX, cube.side, cubeY},   '0');
            }  
        }
        cube.buffer.back() = '\0';
    }

    inline void printCube(SpinningCube const &cube, std::ostream &stream)
    {
        stream << "\x1b[H";
        stream << cube.buffer.data() << '\n';
        stream.flush();
    }

    // thanks to https://stackoverflow.com/a/23370070
#ifdef WIN32
    inline glm::uvec2 getConsoleSize()
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        int columns, rows;
    
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
        return {(unsigned) columns, (unsigned) rows};
    }
#else
    inline glm::uvec2 getConsoleSize()
    {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return {(unsigned) w.ws_col, (unsigned) w.ws_row};
    }
#endif
} // namespace cube
