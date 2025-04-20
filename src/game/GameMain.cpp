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

#define basicLatin text::charRange(L'!', L'~')

void game::gameMain(GLFWwindow *window) 
{
    opengl::ShaderProgram spriteShader{"shaders/colorTexture", true};
    opengl::ShaderProgram plainColorShader{"shaders/plainColor", true};
    opengl::ShaderProgram gridShader{"shaders/grid", true};
    text::Font mainFont{"res/OpenSans-Light.ttf", basicLatin};
    opengl::VertexBuffer quadVBO{sizeof(vertices), vertices};
    Drawable quad = {
        &spriteShader,
        quadVBO,
        opengl::VertexArray{quadVBO, opengl::InterleavedVertexBufferLayout{
            {3, GL_FLOAT},
            {2, GL_FLOAT}
        }},
        {},
        4,
        GL_TRIANGLE_FAN
    };

    glfwSetKeyCallback(window, game::key_callback);

    ecs::getSystemManager().registerSystem<MovementSystem>();
    ecs::getSystemManager().registerSystem<Renderer>();
    ecs::getSystemManager().registerSystem<CameraController>();

    ecs::getComponentManager().registerComponent<Color>();
    ecs::getComponentManager().registerComponent<ModelMatrix>();
    ecs::getComponentManager().registerComponent<Rotation>();
    ecs::getComponentManager().registerComponent<RotationQuaternion>();
    ecs::getComponentManager().registerComponent<opengl::Texture>();
    ecs::getComponentManager().registerComponent<Text>();
    ecs::getComponentManager().registerComponent<Transparent>();

    ecs::Entity_t ballSprite = ecs::makeEntity<Color, Position, Velocity, opengl::Texture, Scale, Drawable>();
    ecs::getSystemManager().addEntity(ballSprite);
    ecs::get<Position>(ballSprite).position = glm::vec3{0, 0, 0};
    ecs::get<Velocity>(ballSprite).velocity = glm::vec3{0, 0, 0};
    ecs::get<Scale   >(ballSprite).scale    = glm::vec3{0.1, 0.1, 0.1};
    ecs::get<Color   >(ballSprite).color    = glm::vec4{0.8, 0.9, 0.1, 1};
    ecs::get<opengl::Texture>(ballSprite) = opengl::Texture("res/textures/ball.png", true, true);
    ecs::get<Drawable>(ballSprite) = quad;
    ecs::get<Drawable>(ballSprite).shader = &spriteShader;

    ecs::Entity_t quad0 = ecs::makeEntity<Color, Position, Rotation, Scale, Drawable, Transparent>();
    ecs::getSystemManager().addEntity(quad0);
    ecs::get<Position>(quad0).position = glm::vec3{0, 0, -1};
    ecs::get<Rotation>(quad0).rotation = glm::vec3{0};
    ecs::get<Scale   >(quad0).scale    = glm::vec3{1};
    ecs::get<Color   >(quad0).color    = glm::vec4{0.8, 0.9, 0.1, 0.5};
    ecs::get<Drawable>(quad0) = quad;
    ecs::get<Drawable>(quad0).shader = &plainColorShader;

    ecs::Entity_t quad1 = ecs::makeEntity<Color, Position, Rotation, Scale, Drawable, Transparent>();
    ecs::getSystemManager().addEntity(quad1);
    ecs::get<Position>(quad1).position = glm::vec3{0, 0, -2};
    ecs::get<Rotation>(quad1).rotation = glm::vec3{0};
    ecs::get<Scale   >(quad1).scale    = glm::vec3{1};
    ecs::get<Color   >(quad1).color    = glm::vec4{0.9, 0.1, 0.8, 0.4};
    ecs::get<Drawable>(quad1) = quad;
    ecs::get<Drawable>(quad1).shader = &plainColorShader;

    ecs::Entity_t quad2 = ecs::makeEntity<Color, Position, Rotation, Scale, Drawable, Transparent>();
    ecs::getSystemManager().addEntity(quad2);
    ecs::get<Position>(quad2).position = glm::vec3{0, 0, -3};
    ecs::get<Rotation>(quad2).rotation = glm::vec3{0};
    ecs::get<Scale   >(quad2).scale    = glm::vec3{1};
    ecs::get<Color   >(quad2).color    = glm::vec4{0.1, 0.9, 0.8, 0.3};
    ecs::get<Drawable>(quad2) = quad;
    ecs::get<Drawable>(quad2).shader = &plainColorShader;

    ecs::Entity_t gridEntity = ecs::makeEntity<Drawable, Color, Transparent>();
    ecs::getSystemManager().addEntity(gridEntity);
    ecs::get<Color>(gridEntity).color = {1, 1, 1, 0.5};
    ecs::get<Drawable>(gridEntity) = {
        &gridShader,
        opengl::VertexBuffer{},
        opengl::VertexArray{opengl::VertexBuffer{}, opengl::VertexBufferLayout{}}, // the vertex buffer is not initialised. the size is 0, no buffer added to the vertex array
        {},
        4,
        GL_TRIANGLE_FAN
    };

    ecs::Entity_t testTextEntity = ecs::makeEntity<Text, Color>();
    ecs::getSystemManager().addEntity(testTextEntity);
    ecs::get<Text>(testTextEntity) = {
        &mainFont,
        "Text rendering\nstill works.",
        {-1, 0.95},
        0.5,
        {}
    };
    ecs::get<Color>(testTextEntity).color = {1, 1, 1, 1};

    ecs::Entity_t cameraEntity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Position, Rotation>(); // QuaternionRotation is also available
    ecs::getSystemManager().addEntity(cameraEntity);
    ecs::get<Position>(cameraEntity).position = {0, 0, 1};
    ecs::get<Rotation>(cameraEntity).rotation = {0, -90, 0};
    ecs::get<ControllableCamera>(cameraEntity) = {
        window,
        2,
        0.1,
        true
    };
    ecs::get<RenderTarget>(cameraEntity) = {};
    ecs::get<RenderTarget>(cameraEntity).clearColor = {0.2, 0.2, 0.2, 1};
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
}
