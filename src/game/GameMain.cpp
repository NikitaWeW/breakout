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

constexpr float CAMERA_SPEED = 10;

using Textures_t = std::initializer_list<std::pair<std::filesystem::path, std::string>>;
template <typename ...Components_t>
ecs::Entity_t createModel(std::filesystem::path const &filepath, int flags = model::LoadFlags::LOAD_DRAWABLE | model::LoadFlags::FLIP_TEXTURES);
void addTextures(ecs::Entity_t const &modelEntity, Textures_t const &textures, bool flipTextures = true);
void registerEcs();
ecs::Entity_t createCamera(GLFWwindow *window);
void addShader(ecs::Entity_t const &entity, opengl::ShaderProgram const &shader);
void loop(GLFWwindow *window);

void game::gameMain(GLFWwindow *window) 
{ // TODO: blend animations, do something when animation.aianimation is nullptr
    opengl::ShaderProgram colorTextureShader{"shaders/colorTexture", true};
    opengl::ShaderProgram plainColorShader{"shaders/plainColor", true};
    opengl::ShaderProgram textureShader{"shaders/colorTexture", true};
    text::Font mainFont{"res/fonts/OpenSans-Light.ttf", basicLatin};
    registerEcs();
    glfwSetKeyCallback(window, game::key_callback);
    auto cache = std::make_unique<Cache>();
    globalCache = &*cache;
    // ====================

    ecs::Entity_t modelEntity = createModel<opengl::ShaderProgram>("res/models/suzanne.glb");
    addTextures(modelEntity, {
        {"res/materials/wood1/Wood049_1K-JPG_Color.jpg", "diffuse"},
        {"res/materials/wood1/Wood049_1K-JPG_NormalGL.jpg", "normal"},
        {"res/materials/wood1/Wood049_1K-JPG_Roughness.jpg", "rough"}
    });
    addShader(modelEntity, colorTextureShader);

    // ecs::Entity_t cameraEntity = 
    createCamera(window);
    
    loop(window);
}

template <typename... Components_t>
ecs::Entity_t createModel(std::filesystem::path const &filepath, int flags)
{
    if(game::globalCache->modelCache.find(filepath) == game::globalCache->modelCache.end()) {
        game::globalCache->modelCache.insert(std::make_pair(filepath, model::Model{filepath, flags}));
    }
    model::Model &model = game::globalCache->modelCache.at(filepath);

    ecs::Entity_t entity = ecs::makeEntity<model::Model, Components_t...>();
    ecs::get<model::Model>(entity) = model;

    if(model.getScene()->HasAnimations()) {
        game::Animation animation;

        ecs::addComponent<game::Animation>(entity);
        ecs::get<game::Animation>(entity) = animation;
    }
    ecs::getSystemManager().addEntity(entity);

    return entity;
}
void loop(GLFWwindow *window)
{
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
void addTextures(ecs::Entity_t const &modelEntity, Textures_t const &textures, bool flipTextures)
{
    if(!ecs::entityHasComponent<model::Model>(modelEntity)) return;
    model::Model &model = ecs::get<model::Model>(modelEntity);

    for(auto const & [path, type] : textures) {
        opengl::Texture *texture;
        if(game::globalCache->textureCache.find(path) != game::globalCache->textureCache.end()) {
            texture = &game::globalCache->textureCache.at(path);
        } else {
            game::globalCache->textureCache.insert({path, opengl::Texture{path, flipTextures, type == "diffuse", GL_NEAREST, GL_CLAMP_TO_EDGE, type}});
            texture = &game::globalCache->textureCache.rbegin()->second;
        }
        assert(texture);
        for(model::Mesh &mesh : model.getMeshes()) {
            mesh.textures.push_back(*texture);
            game::globalCache->textureCache.rbegin()->second.type = type;
        }
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
    ecs::Entity_t cameraEntity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Position, Rotation>();
    ecs::get<Position>(cameraEntity).position = {0, 0, 1};
    ecs::get<Rotation>(cameraEntity).rotation = {0, -90, 0};
    ecs::get<ControllableCamera>(cameraEntity) = {
        window,
        CAMERA_SPEED,
        0.1,
        true
    };
    ecs::get<RenderTarget>(cameraEntity) = {};
    ecs::get<RenderTarget>(cameraEntity).clearColor = {0.2, 0.2, 0.2, 1};
    ecs::get<Camera>(cameraEntity) = {};

    ecs::getSystemManager().addEntity(cameraEntity);
    return cameraEntity;
}
void addShader(ecs::Entity_t const &entity, opengl::ShaderProgram const &shader)
{
    if(!ecs::entityHasComponent<opengl::ShaderProgram>(entity)) {
        ecs::addComponent<opengl::ShaderProgram>(entity);
    }
    ecs::get<opengl::ShaderProgram>(entity) = shader;
}

game::Cache *game::globalCache = nullptr;