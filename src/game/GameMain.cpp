#include "GameMain.hpp"
#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Animator.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>
#include <memory>
#include "utils/Model.hpp"
#include "LevelParcer.hpp"

constexpr float CAMERA_SPEED = 10;

void registerEcs();
ecs::Entity_t createCamera(GLFWwindow *window);
void loop(GLFWwindow *window);

void game::gameMain(GLFWwindow *window) 
{ // TODO: blend animations, do something when animation.aianimation is nullptr
    opengl::ShaderProgram propShader{"shaders/prop", true};
    text::Font mainFont{"res/fonts/OpenSans-Light.ttf", basicLatin};
    registerEcs();
    glfwSetKeyCallback(window, game::key_callback);
    // ====================

    ecs::Entity_t windowEntity = ecs::makeEntity<Window>();
    ecs::get<Window>(windowEntity) = {
        .glfwwindow = window
    };
    ecs::getSystemManager().getEntities().insert(windowEntity);

    LevelParcer parcer;
    auto sceneEntities = parcer.parceScene("res/levels-scenes/plane.json");
    if(parcer.getErrorString() != "") {
        std::cout << "failed to load scene at \"res/levels-scenes/plane.json\": " << parcer.getErrorString() << '\n';
    }
    if(sceneEntities.has_value()) {
        for(ecs::Entity_t const &entity : sceneEntities.value()) {
            if(ecs::entityHasComponent<model::Model>(entity)) {
                ecs::addComponent(entity, propShader);
            }
            ecs::getSystemManager().getEntities().insert(entity);
        }
    }
    
    // ! all deltatime is in seconds
    double deltatime = 0.0001;
    std::thread fpsShower{
        [&deltatime, &window]() {
            while(!glfwWindowShouldClose(window)) {
                glfwSetWindowTitle(window, ("breakout -- " + std::to_string((int) std::round(1 / deltatime)) + " FPS").c_str()); 
                std::this_thread::sleep_for(std::chrono::milliseconds{500}); 
            }
        }  
    }; 
    fpsShower.detach();
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        ecs::getSystemManager().update(deltatime);

        glfwSwapBuffers(window);
        glfwPollEvents();
        deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * 1.0E-6;
    }
}

void registerEcs()
{
    using namespace game;
    ecs::getSystemManager().registerSystem<Renderer>();
    ecs::getSystemManager().registerSystem<CameraController>();
    ecs::getSystemManager().registerSystem<Animator>();
    // ====================
    ecs::getComponentManager().registerComponent<Color>();
    ecs::getComponentManager().registerComponent<Position>();
    ecs::getComponentManager().registerComponent<Window>();
    ecs::getComponentManager().registerComponent<ModelMatrix>();
    ecs::getComponentManager().registerComponent<OrientationEuler>();
    ecs::getComponentManager().registerComponent<OrientationQuaternion>();
    ecs::getComponentManager().registerComponent<opengl::ShaderProgram>();
    ecs::getComponentManager().registerComponent<opengl::Texture>();
    ecs::getComponentManager().registerComponent<Text>();
    ecs::getComponentManager().registerComponent<Scale>();
    ecs::getComponentManager().registerComponent<AnimationTransition>();
    ecs::getComponentManager().registerComponent<RepeatTexture>();
    ecs::getComponentManager().registerComponent<Animation>();
}