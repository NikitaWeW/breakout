#include "LevelParcer.hpp"
#include "Physics.hpp"
#include "Controller.hpp"
#include "json.hpp"
#include "Animator.hpp"
using json = nlohmann::json;

game::LevelParcer::~LevelParcer() = default;

ecs::Entity_t game::LevelParcer::createModel(std::filesystem::path const &filepath, bool flipWindingOrder, bool flipTextures) {
    if(m_modelCache.find(filepath) == m_modelCache.end()) {
        m_modelCache.insert({filepath, model::Model{filepath, 
            model::FLIP_TEXTURES | 
            model::LOAD_DRAWABLE | 
            (flipWindingOrder ? model::FLIP_WINDING_ORDER : 0) | 
            (flipTextures ? model::FLIP_TEXTURES : 0)}});
    }
    model::Model &model = m_modelCache.at(filepath);

    ecs::Entity_t entity = ecs::makeEntity<model::Model>();
    ecs::get<model::Model>(entity) = model;

    if(model.getScene()->HasAnimations()) {
        game::Animation animation;

        ecs::addComponent<game::Animation>(entity);
        ecs::get<game::Animation>(entity) = animation;
    }

    return entity;
}
void game::LevelParcer::addTexture(ecs::Entity_t const &modelEntity, std::filesystem::path const &path, std::string const &type, bool flipTextures)
{
    if(!ecs::entityHasComponent<model::Model>(modelEntity)) return;
    model::Model &model = ecs::get<model::Model>(modelEntity);

    if(m_textureCache.find(path) == m_textureCache.end()) {
        m_textureCache.insert({path, opengl::Texture{path, flipTextures, type == "diffuse", type}});
    }
    opengl::Texture &texture = m_textureCache.at(path);
    texture.type = type;
    for(model::Mesh &mesh : model.getMeshes()) {
        mesh.textures.push_back(texture);
    }
}
template<size_t L = 3>
glm::vec<L, float> getVecFromJSON(json const &jsonObj) {
    assert(jsonObj.is_array());
    assert(jsonObj.at(0).is_number());

    glm::vec<L, float> result;
    for(size_t i = 0; i < L; ++i) {
        result[i] = jsonObj.at(i);
    }

    return result;
}
GLFWwindow * findWindow() {
    auto iter = std::find_if(
        ecs::getSystemManager().getEntities().cbegin(), 
        ecs::getSystemManager().getEntities().cend(), 
        [](ecs::Entity_t const &entity){ 
            return ecs::entityHasComponent<game::Window>(entity); 
        }
    );
    if(iter == ecs::getSystemManager().getEntities().cend()) {
        return nullptr;
    } else {
        return ecs::get<game::Window>(*iter).glfwwindow;
    }
}

std::optional<std::vector<ecs::Entity_t>> game::LevelParcer::parceScene(std::filesystem::path const &filepath)
{
    assert(filepath.extension().string() == ".json");
    std::ifstream filestream{filepath};
    if(!filestream) {
        m_errorStr = "failed to open file";
    }
    std::optional<std::vector<ecs::Entity_t>> result;
    
    json data = json::parse(filestream);
    if(data.find("entities") != data.end()) {
        result.emplace();
        json jsonentities = data["entities"];
        for(auto entityIter = jsonentities.cbegin(); entityIter != jsonentities.cend(); ++entityIter) {
            json const &jsonentity = entityIter.value();
            ecs::Entity_t entity;
            std::string type = jsonentity.find("type") != jsonentity.end() && jsonentity.at("type").is_string() ? jsonentity["type"].get<std::string>() : "";
            if(type == "prop") {
                std::filesystem::path path = jsonentity.find("path") != jsonentity.end() && jsonentity.at("path").is_string() ? jsonentity["path"].get<std::string>() : "";
                if(path == "") {
                    m_errorStr.append("\nno path specified for prop");
                    continue;
                } 
                bool flipTextures = 
                    jsonentity.find("flip textures") != jsonentity.end() && jsonentity.at("flip textures").is_boolean() ? 
                    jsonentity["flip textures"].get<bool>() :
                    true;
                bool flipWindingOrder = 
                    jsonentity.find("flip winding order") != jsonentity.end() && jsonentity.at("flip winding order").is_boolean() ? 
                    jsonentity["flip winding order"].get<bool>() :
                    false;
                entity = createModel(path, flipWindingOrder, flipTextures);

                if(jsonentity.find("textures") != jsonentity.end()) {
                    json jsontextures = jsonentity["textures"];
                    for(auto textureIter = jsontextures.cbegin(); textureIter != jsontextures.cend(); ++textureIter) {
                        json jsontexture = textureIter.value();
                        std::string type = jsontexture.find("type") != jsontexture.end() && jsontexture.at("type").is_string() ? jsontexture["type"].get<std::string>() : "diffuse";
                        std::filesystem::path path = jsontexture.find("path") != jsontexture.end() && jsontexture.at("path").is_string() ? jsontexture["path"].get<std::string>() : "";

                        if(path == "") {
                            m_errorStr.append("\nno path specified for texture");
                            continue;
                        }
                        addTexture(entity, path, type, flipTextures);
                    }
                }
                if(jsonentity.find("position") != jsonentity.end() && jsonentity.at("position").is_array()) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.find("rotation") != jsonentity.end() && jsonentity.at("rotation").is_array()) {
                    ecs::addComponent<game::OrientationEuler>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["rotation"]))});
                }
                if(jsonentity.find("scale") != jsonentity.end() && jsonentity.at("scale").is_array()) {
                    ecs::addComponent<game::Scale>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["scale"]))});
                }
                if(jsonentity.find("repeat textures") != jsonentity.end() && jsonentity.at("repeat textures").is_number()) {
                    ecs::addComponent(entity, RepeatTexture{jsonentity["repeat textures"].get<unsigned>()});
                }
            } else if(type == "controllable camera") {
                GLFWwindow *window = findWindow();
                if(!window) {
                    m_errorStr.append("\nwindow not found for controllable camera");
                    continue;
                }
                entity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Window>();
                ecs::get<ControllableCamera>(entity) = {
                    .speedUnitsPerSecond = jsonentity.find("speed") != jsonentity.end() && jsonentity.at("speed").is_number() ? jsonentity["speed"].get<float>() : 1,
                    .sensitivity = 0.1,
                    .locked = true
                };
                ecs::get<RenderTarget>(entity) = {};
                ecs::get<RenderTarget>(entity).clearColor = {0.2, 0.2, 0.2, 1};
                ecs::get<Camera>(entity) = {};
                ecs::get<Window>(entity) = {window};

                if(jsonentity.find("position") != jsonentity.end()) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.find("rotation") != jsonentity.end()) {
                    ecs::addComponent<game::OrientationEuler>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["rotation"]))});
                }
            } else if(type == "point light") {
                entity = ecs::makeEntity<Light, PointLight>();
                ecs::get<Light>(entity) = {
                    .color = jsonentity.find("color") != jsonentity.end() && jsonentity.at("color").is_array() ? static_cast<glm::vec3>(getVecFromJSON(jsonentity["color"])) : glm::vec3{1},
                    .attenuation = jsonentity.find("attenuation") != jsonentity.end() && jsonentity.at("attenuation").is_number() ? jsonentity.at("attenuation").get<float>() : 10.0f
                };
                if(jsonentity.find("position") != jsonentity.end()) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
            } else {
                m_errorStr.append("uncrecognised type: " + type);
            }
            result.value().push_back(entity);
        }
    }
    return result;
}
