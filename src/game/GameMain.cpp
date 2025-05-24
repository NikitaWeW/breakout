#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Animator.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>
#include <memory>
#include "utils/Model.hpp"
#include "LevelParser.hpp"

constexpr float CAMERA_SPEED = 10;

void registerEcs();
#define basicLatin text::charRange(L'!', L'~')

namespace game
{
    void gameMain(GLFWwindow *mainWindow);
} // namespace game

void game::gameMain(GLFWwindow *window) 
{
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

    LevelParser parcer;
    auto sceneEntities = parcer.parseScene("res/scenes/plane.json");
    if(parcer.getErrorString() != "") {
        std::cout << "failed to load scene at \"res/scenes/plane.json\": " << parcer.getErrorString() << '\n';
    }
    if(sceneEntities.has_value()) {
        for(ecs::Entity_t const &entity : sceneEntities.value()) {
            if(ecs::entityHasComponent<model::Model>(entity)) {
                ecs::addComponent(entity, propShader);
            }
            ecs::getSystemManager().getEntities().insert(entity);
        }
    }
    ecs::Entity_t lightStorageEntity = ecs::makeEntity<LightUBO, LightUpdater::LightStorage>();
    ecs::get<LightUBO>(lightStorageEntity) = {};
    ecs::get<LightUpdater::LightStorage>(lightStorageEntity) = {};
    ecs::getSystemManager().getEntities().insert(lightStorageEntity);
    
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
    ecs::getSystemManager().registerSystem<LightUpdater>();
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
    ecs::getComponentManager().registerComponent<Direction>();
    ecs::getComponentManager().registerComponent<MaterialProperties>();
}