#include "GameMain.hpp"
#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>
#include "utils/Model.hpp"

float vertices[] = {
    -0.5f,  0.5f, -0.5f,  0, 1,
     0.5f,  0.5f, -0.5f,  1, 1,
     0.5f, -0.5f, -0.5f,  1, 0,
    -0.5f, -0.5f, -0.5f,  0, 0 
};

#define basicLatin text::charRange(L'!', L'~')

void game::gameMain(GLFWwindow *window) 
{
    opengl::ShaderProgram colorTextureShader{"shaders/colorTexture", true};
    opengl::ShaderProgram plainColorShader{"shaders/plainColor", true};
    opengl::ShaderProgram textureShader{"shaders/colorTexture", true};
    text::Font mainFont{"res/OpenSans-Light.ttf", basicLatin};
    opengl::VertexBuffer quadVBO{sizeof(vertices), vertices};
    Drawable quad;
    quad.vb = quadVBO;
    quad.va = opengl::VertexArray{quadVBO, opengl::InterleavedVertexBufferLayout{
        {3, GL_FLOAT},
        {2, GL_FLOAT}
    }};
    quad.ib = {};
    quad.count = 4;
    quad.mode = GL_TRIANGLE_FAN;
    model::Model model{"res/models/backpack/backpack.obj", model::ModelLoadFlags::LOAD_DATA | model::ModelLoadFlags::LOAD_DRAWABLE};
    model::Model gridModel{};
    gridModel.getMeshes().push_back({
        {}, // data. optional
        Drawable{ // drawable
            opengl::VertexBuffer{},
            opengl::VertexArray{opengl::VertexBuffer{}, opengl::VertexBufferLayout{}}, // the vertex buffer is not initialised. the size is 0, no buffer added to the vertex array
            {},
            4,
            GL_TRIANGLE_FAN
        },
        {} // textures
    });

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
    ecs::getComponentManager().registerComponent<Velocity>();
    ecs::getComponentManager().registerComponent<Scale>();

    ecs::Entity_t gridEntity = ecs::makeEntity<model::Model, Color, Transparent, opengl::ShaderProgram>();
    ecs::getSystemManager().addEntity(gridEntity);
    ecs::get<Color>(gridEntity).color = {1, 1, 1, 0.5};
    ecs::get<model::Model>(gridEntity) = gridModel;
    ecs::get<opengl::ShaderProgram>(gridEntity) = opengl::ShaderProgram{"shaders/grid", true};
    
    ecs::Entity_t cubeEntity = ecs::makeEntity<model::Model, opengl::ShaderProgram>();
    ecs::getSystemManager().addEntity(cubeEntity);
    assert(model.getMeshes()[0].drawable.has_value());
    ecs::get<model::Model>(cubeEntity) = model;
    ecs::get<opengl::ShaderProgram>(cubeEntity) = textureShader;

    ecs::Entity_t cameraEntity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Position, Rotation>(); // QuaternionRotation is also available
    ecs::getSystemManager().addEntity(cameraEntity);
    ecs::get<Position>(cameraEntity).position = {0, 0, 1};
    ecs::get<Rotation>(cameraEntity).rotation = {0, -90, 0};
    ecs::get<ControllableCamera>(cameraEntity) = {
        window,
        4,
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
