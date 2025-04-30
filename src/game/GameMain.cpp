#include "GameMain.hpp"
#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Animator.hpp"
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
    text::Font mainFont{"res/fonts/OpenSans-Light.ttf", basicLatin};
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
    typedef model::Model::LoadFlags Flags;
    model::Model model{"res/models/dancing_vampire/dancing_vampire.dae", Flags::LOAD_DATA | Flags::LOAD_DRAWABLE | Flags::FLIP_TEXTURES};
    glfwSetKeyCallback(window, game::key_callback);

    ecs::getSystemManager().registerSystem<Renderer>();
    ecs::getSystemManager().registerSystem<CameraController>();
    ecs::getSystemManager().registerSystem<Animator>();

    ecs::Entity_t modelEntity = ecs::makeEntity<Scale, model::Model, opengl::ShaderProgram, Animation>();
    ecs::getSystemManager().addEntity(modelEntity);
    assert(model.getMeshes()[0].drawable.has_value());
    ecs::get<model::Model>(modelEntity) = model;
    ecs::get<Scale>(modelEntity).scale = glm::vec3{0.1};
    ecs::get<opengl::ShaderProgram>(modelEntity) = textureShader;
    Animation &animation = ecs::get<Animation>(modelEntity);
    animation.aianimation = model.getScene()->HasAnimations() ? model.getScene()->mAnimations[0] : nullptr;
    animation.repeatMode = MIRROR;

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
    ecs::getComponentManager().registerComponent<Color>();
    ecs::getComponentManager().registerComponent<ModelMatrix>();
    ecs::getComponentManager().registerComponent<Rotation>();
    ecs::getComponentManager().registerComponent<RotationQuaternion>();
    ecs::getComponentManager().registerComponent<opengl::Texture>();
    ecs::getComponentManager().registerComponent<Text>();
    ecs::getComponentManager().registerComponent<Scale>();
    ecs::getComponentManager().registerComponent<Animation>();
    // ========================================================

    // ! all deltatime is in seconds
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
