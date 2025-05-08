#include "GameMain.hpp"
#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Animator.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>
#include "utils/Model.hpp"

constexpr float CAMERA_SPEED = 10;

void registerEcs()
{
    using namespace game;
    ecs::getSystemManager().registerSystem<Renderer>();
    ecs::getSystemManager().registerSystem<CameraController>();
    ecs::getSystemManager().registerSystem<Animator>();
    // ====================
    ecs::getComponentManager().registerComponent<Color>();
    ecs::getComponentManager().registerComponent<ModelMatrix>();
    ecs::getComponentManager().registerComponent<Rotation>();
    ecs::getComponentManager().registerComponent<RotationQuaternion>();
    ecs::getComponentManager().registerComponent<opengl::Texture>();
    ecs::getComponentManager().registerComponent<Text>();
    ecs::getComponentManager().registerComponent<Scale>();
    ecs::getComponentManager().registerComponent<AnimationTransition>();
    ecs::getComponentManager().registerComponent<Animation>();
}
ecs::Entity_t createCamera(GLFWwindow *window)
{
    using namespace game;
    ecs::Entity_t cameraEntity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Position, RotationQuaternion>(); // QuaternionRotation is also available
    ecs::get<Position>(cameraEntity).position = {0, 0, 1};
    // ecs::get<Rotation>(cameraEntity).rotation = {0, -90, 0};
    ecs::get<ControllableCamera>(cameraEntity) = {
        window,
        CAMERA_SPEED,
        0.1,
        true
    };
    ecs::get<RenderTarget>(cameraEntity) = {};
    ecs::get<RenderTarget>(cameraEntity).clearColor = {0.2, 0.2, 0.2, 1};
    ecs::get<Camera>(cameraEntity) = {};

    return cameraEntity;
}
template <typename ...Components_t>
ecs::Entity_t createModel(std::filesystem::path const &filepath, int flags = model::LoadFlags::LOAD_DRAWABLE | model::LoadFlags::FLIP_TEXTURES)
{
    static std::map<std::filesystem::path, model::Model> modelCache;

    if(modelCache.find(filepath) == modelCache.end()) {
        modelCache.insert(std::make_pair(filepath, model::Model{filepath, flags}));
    }
    model::Model &model = modelCache.at(filepath);

    ecs::Entity_t entity = ecs::makeEntity<model::Model, Components_t...>();
    ecs::get<model::Model>(entity) = model;

    if(model.getScene()->HasAnimations()) {
        game::Animation animation;

        ecs::addComponent<game::Animation>(entity);
        ecs::get<game::Animation>(entity) = animation;
    }

    return entity;
}

void game::gameMain(GLFWwindow *window) 
{ // TODO: blend animations, do something when animation.aianimation is nullptr
    opengl::ShaderProgram colorTextureShader{"shaders/colorTexture", true};
    opengl::ShaderProgram plainColorShader{"shaders/plainColor", true};
    opengl::ShaderProgram textureShader{"shaders/colorTexture", true};
    text::Font mainFont{"res/fonts/OpenSans-Light.ttf", basicLatin};
    registerEcs();
    // ====================

    ecs::Entity_t modelEntity = createModel<opengl::ShaderProgram, Scale>("res/models/gorilla/Gorilla_hd.fbx");
    ecs::getSystemManager().addEntity(modelEntity);
    ecs::get<opengl::ShaderProgram>(modelEntity) = colorTextureShader;
    ecs::get<Scale>(modelEntity).scale = glm::vec3{1};
    ecs::get<game::Animation>(modelEntity).aianimation = ecs::get<model::Model>(modelEntity).getScene()->mAnimations[0];

    ecs::Entity_t cameraEntity = createCamera(window);
    ecs::getSystemManager().addEntity(cameraEntity);
    
    // ! all deltatime is in seconds
    double deltatime = 0.0001;
    std::thread test{[&](){
        std::this_thread::sleep_for(std::chrono::seconds{2});
        std::cout << "chaning animation from " << ecs::get<model::Model>(modelEntity).getScene()->mAnimations[0]->mName.C_Str() << " to " << ecs::get<model::Model>(modelEntity).getScene()->mAnimations[1]->mName.C_Str() << "...\n";
        AnimationTransition transition;
        transition.to.aianimation = ecs::get<model::Model>(modelEntity).getScene()->mAnimations[1];
        const float changeTime = 0.5;
        transition.factorPerSecond = 1.0 / changeTime;
        transition.easeFunction = easeFunc::inExpo;
        ecs::addComponent(modelEntity, transition);
        std::this_thread::sleep_for(std::chrono::milliseconds{(int) (changeTime * 1000)});
        std::cout << "should have changed!\n";
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }}; test.detach();
        
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
