#include "glm/glm.hpp"
#include "Renderer.hpp"
#include "Physics.hpp"
#include "Animator.hpp"
#include "Controller.hpp"
#include <chrono>
#include <thread>
#include <memory>
#include <random>
#include <iostream>
#include "utils/Model.hpp"
#include "LevelParser.hpp"

void registerEcs();

namespace game
{
    void gameMain(GLFWwindow *mainWindow);
} // namespace game

ecs::Entity_t makeSceneEntity(std::filesystem::path const &filepath)
{
    ecs::Entity_t result = ecs::makeEntity<game::Scene>();
    game::Scene &scene = ecs::get<game::Scene>(result);
    scene = game::getLevelParser().parseScene(filepath);
    if(scene.containedEntities.size() == 0) {
        std::cout << "failed to load scene \"" << filepath << "\"!\n";
    }
    if(game::getLevelParser().getErrorString() != "") {
        std::cout << game::getLevelParser().getErrorString() << '\n';
    }
    ecs::getSystemManager().getEntities().insert(scene.containedEntities.begin(), scene.containedEntities.end());
    return result;
}
ecs::Entity_t makeWindowEntity(GLFWwindow *window) 
{
    ecs::Entity_t windowEntity = ecs::makeEntity<game::Window>();
    ecs::get<game::Window>(windowEntity) = {
        .glfwwindow = window
    };
    return windowEntity;
}
ecs::Entity_t makeLightStorageEntity() 
{
    using namespace game;
    ecs::Entity_t lightStorageEntity = ecs::makeEntity<LightUBO, LightUpdater::LightStorage>();
    ecs::get<LightUBO>(lightStorageEntity) = {};
    ecs::get<LightUpdater::LightStorage>(lightStorageEntity) = {};
    return lightStorageEntity;
}

void game::gameMain(GLFWwindow *window) 
{
    registerEcs();
    glfwSetKeyCallback(window, game::key_callback);
    glfwSetMouseButtonCallback(window, game::mouse_button_callback);
    glfwSetScrollCallback(window, game::scroll_callback);
    glfwSetCursorPosCallback(window, game::cursor_position_callback);

    ecs::getSystemManager().getEntities().insert(makeWindowEntity(window));
    // ecs::getSystemManager().getEntities().insert(makeSceneEntity("res/scenes/plane.json"));
    ecs::getSystemManager().getEntities().insert(makeSceneEntity("res/scenes/sponza.json"));
    ecs::getSystemManager().getEntities().insert(makeLightStorageEntity());
    
    // ====================

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
    ecs::getComponentManager().registerComponent<model::Model>();
    ecs::getComponentManager().registerComponent<Light>();
    ecs::getComponentManager().registerComponent<PointLight>();
    ecs::getComponentManager().registerComponent<SpotLight>();
    ecs::getComponentManager().registerComponent<DirectionalLight>();
    ecs::getComponentManager().registerComponent<game::Transparent>();
    ecs::getComponentManager().registerComponent<game::SemiTransparent>();
}