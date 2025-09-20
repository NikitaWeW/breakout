#include "cooload.hpp"
#include "glm/gtc/matrix_transform.hpp"

// for console size
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#endif

void cooload::resizeCube(SpinningCube &cube)
{
    cube.zbuffer.resize(cube.imageSize.x * cube.imageSize.y);
    cube.buffer.resize(cube.imageSize.x * cube.imageSize.y + 1);
}

void cooload::animateCube(SpinningCube &cube, float time)
{
    cube.modelMat = glm::rotate(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -cube.distanceFromCam}), time, cube.rotationAxis);
}

static void calculateForSurface(cooload::SpinningCube &cube, glm::vec3 pos, char ch)
{
    glm::vec4 p = cube.projMat * cube.modelMat * glm::vec4{pos, 1};
    p /= p.w;
    p.x = (p.x * 0.5 + 0.5) * cube.viewport.size.x + cube.viewport.position.x;
    p.y = (p.y * 0.5 + 0.5) * cube.viewport.size.y + cube.viewport.position.y;
    
    size_t idx = static_cast<int>(p.x) + static_cast<int>(p.y) * cube.imageSize.x;
    if (idx >= 0 && idx < cube.imageSize.x * cube.imageSize.y) {
        if (p.z < cube.zbuffer[idx]) {
            cube.zbuffer[idx] = p.z;
            cube.buffer[idx] = ch;
        }
    }

    if(p.y < cube.imageSize.y)
    {
        cube.buffer[cube.imageSize.x + static_cast<int>(p.y) * cube.imageSize.x] = '\n';
    }
}

void cooload::draw(SpinningCube &cube)
{
    std::fill(cube.buffer.begin(), cube.buffer.end(), cube.background);
    std::fill(cube.zbuffer.begin(), cube.zbuffer.end(), 1.0f);

    cube.projMat = glm::perspective<float>(45, cube.cellAspect * ((float) cube.viewport.size.x / cube.viewport.size.y), 0.01, 250);

    for(float cubeX = -cube.side; cubeX < cube.side; cubeX += cube.increment)
    {
        for(float cubeY = -cube.side; cubeY < cube.side; cubeY += cube.increment)
        {
            calculateForSurface(cube, {cubeX, cubeY, -cube.side},  cube.faces[0]);
            calculateForSurface(cube, {cube.side, cubeY, cubeX},   cube.faces[1]);
            calculateForSurface(cube, {-cube.side, cubeY, -cubeX}, cube.faces[2]);
            calculateForSurface(cube, {-cubeX, cubeY, cube.side},  cube.faces[3]);
            calculateForSurface(cube, {cubeX, -cube.side, -cubeY}, cube.faces[4]);
            calculateForSurface(cube, {cubeX, cube.side, cubeY},   cube.faces[5]);
        }  
    }
    cube.buffer.back() = '\0';
}

#ifdef _WIN32
// thanks to https://stackoverflow.com/a/23370070
glm::uvec2 cooload::getConsoleSize()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return {(unsigned) columns, (unsigned) rows};
}
void cooload::gotoxy(glm::uvec2 pos) 
{
    COORD coord;
    coord.X = pos.x;
    coord.Y = pos.y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
#else
glm::uvec2 cooload::getConsoleSize()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return {(unsigned) w.ws_col, (unsigned) w.ws_row};
}
void cooload::gotoxy(glm::uvec2 pos)
{
    std::cout << "\033[" << pos.y + 1 << ";" << pos.x + 1 << "H";
    std::cout.flush();
}
#endif

void cooload::draw(Bar &bar)
{
    bar.buffer.str("");

    unsigned totalWidth = glm::max(static_cast<int>(bar.width) - static_cast<int>(bar.begin.size()) - static_cast<int>(bar.end.size()) - 5, 0);
    unsigned filledWidth = static_cast<unsigned>(static_cast<float>(totalWidth) * bar.percentage);

    bar.buffer << bar.begin;

    if(filledWidth != 0)
    {
        for(unsigned i = 0; i < filledWidth - 1; ++i) 
        {
            bar.buffer << bar.ch;
        }
        bar.buffer << bar.head;
    }
    for(unsigned i = filledWidth; i < totalWidth; ++i) 
    {
        bar.buffer << bar.empty;
    }
    bar.buffer << bar.end;
    if(bar.printPercentage)
    {
        bar.buffer << ' ' << glm::round(bar.percentage * 100) << '%';
    }
}
void cooload::clearConsole()
{
#ifdef _WIN32 // Check if compiling on Windows
    system("cls");
#else // Assume Unix-based system otherwise
    system("clear");
#endif
    std::cout.flush();
}
