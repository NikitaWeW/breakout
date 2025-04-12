#include "GameMain.hpp"
#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>

float vertices[] = { // TODO: model loading once again
    -0.5f,  0.5f, -0.5f,  0, 1,
     0.5f,  0.5f, -0.5f,  1, 1,
     0.5f, -0.5f, -0.5f,  1, 0,
    -0.5f, -0.5f, -0.5f,  0, 0 
};
unsigned indices[] = {
    0, 1, 2,
    0, 2, 3
};

int game::gameMain(GLFWwindow *window) {
    opengl::ShaderProgram spriteShader{"shaders/colorTexture", true};

    ecs::getSystemManager().registerSystem<MovementSystem>();
    ecs::getSystemManager().registerSystem<Renderer>();
    ecs::getSystemManager().registerSystem<CameraController>();

    ecs::getComponentManager().registerComponent<Color>();
    ecs::getComponentManager().registerComponent<ModelMatrix>();
    ecs::getComponentManager().registerComponent<Rotation>();
    ecs::getComponentManager().registerComponent<RotationQuaternion>();
    ecs::getComponentManager().registerComponent<opengl::Texture>();

    ecs::Entity_t ballSprite = ecs::makeEntity<Color, Position, Velocity, opengl::Texture, Scale, Drawable>();
    ecs::getSystemManager().addEntity(ballSprite);

    ecs::get<Position>(ballSprite).position = glm::vec3{0, 0, 0};
    ecs::get<Velocity>(ballSprite).velocity = glm::vec3{0, 0, 0};
    ecs::get<Scale   >(ballSprite).scale    = glm::vec3{0.1, 0.1, 0.1};
    ecs::get<Color   >(ballSprite).color    = glm::vec4{0.8, 0.9, 0.1, 1};
    ecs::get<opengl::Texture>(ballSprite) = opengl::Texture("res/textures/ball.png", true, true);
    {
        opengl::VertexBuffer vb{sizeof(vertices), vertices};
        ecs::get<Drawable>(ballSprite) = {
            &spriteShader,
            vb,
            opengl::VertexArray{vb, opengl::InterleavedVertexBufferLayout{
                {3, GL_FLOAT},
                {2, GL_FLOAT}
            }},
            opengl::IndexBuffer{sizeof(indices), indices},
            6,
            GL_TRIANGLES
        };
    }

    ecs::Entity_t cameraEntity = ecs::makeEntity<Camera, ControllableCamera, Position, Rotation>();
    ecs::getSystemManager().addEntity(cameraEntity);
    ecs::get<Position>(cameraEntity).position = {0, 0, 1};
    ecs::get<Rotation>(cameraEntity).rotation = {0, -90, 0};
    ecs::get<ControllableCamera>(cameraEntity) = {
        window,
        1,
        100,
        true
    };
    ecs::get<Camera>(cameraEntity) = {};

    // ========================================================

    double deltatime = 1;
    std::thread fpsShower{[&deltatime, &window](){while(!glfwWindowShouldClose(window)) {glfwSetWindowTitle(window, ("breakout -- " + std::to_string((int) std::round(1 / deltatime)) + " FPS").c_str()); std::this_thread::sleep_for(std::chrono::milliseconds{500}); }}}; fpsShower.detach();
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        ecs::getSystemManager().update(deltatime);

        glfwSwapBuffers(window);
        glfwPollEvents();
        deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1.0E-6;
    }

    return 0;
}
