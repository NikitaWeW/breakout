#include "LevelParser.hpp"
#include "Physics.hpp"
#include "Controller.hpp"
#include "json.hpp"
#include "Animator.hpp"
using json = nlohmann::json;

game::LevelParser::~LevelParser() = default;

// pair(model entity, set of light entities)
std::pair<ecs::Entity_t, std::set<ecs::Entity_t>> game::LevelParser::createModel(std::filesystem::path const &filepath, bool flipWindingOrder, bool flipTextures) {
    if(m_modelCache.find(filepath) == m_modelCache.end()) {
        m_modelCache.insert({filepath, model::Model{filepath, 
            model::FLIP_TEXTURES | 
            model::LOAD_DRAWABLE | 
            (flipWindingOrder ? model::FLIP_WINDING_ORDER : 0) | 
            (flipTextures ? model::FLIP_TEXTURES : 0)}});
    }
    model::Model &model = m_modelCache.at(filepath);

    ecs::Entity_t modelEntity = ecs::makeEntity<model::Model>();
    ecs::get<model::Model>(modelEntity) = model;

    if(model.getScene()->HasAnimations()) {
        game::Animation animation;

        ecs::addComponent<game::Animation>(modelEntity);
        ecs::get<game::Animation>(modelEntity) = animation;
    }
    std::set<ecs::Entity_t> lights;
    for(size_t i = 0; i < model.getScene()->mNumLights; ++i) {
        ecs::Entity_t lightEntity = ecs::makeEntity<Light, PointLight, Position>();
        aiLight const *assimpLight = model.getScene()->mLights[i];

        if(assimpLight->mType == aiLightSource_POINT) {
            ecs::get<Light>(lightEntity) = {
                .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
            };
            ecs::get<PointLight>(lightEntity) = {
                .attenuation = assimpLight->mAttenuationQuadratic
            };
            ecs::get<Position>(lightEntity).position = glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z};
            lights.insert(lightEntity);
        } else if(assimpLight->mType == aiLightSource_DIRECTIONAL) {
            ecs::Entity_t lightEntity = ecs::makeEntity<Light, DirectionalLight, Direction>();
            ecs::get<Light>(lightEntity) = {
                .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
            };
            ecs::get<Direction>(lightEntity).dir = glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z};
            lights.insert(lightEntity);
        } else if(assimpLight->mType == aiLightSource_SPOT) {
            ecs::Entity_t lightEntity = ecs::makeEntity<Light, SpotLight, Position, Direction>();
            ecs::get<Light>(lightEntity) = {
                .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
            };
            ecs::get<SpotLight>(lightEntity) = {
                .innerConeAngle = glm::degrees(assimpLight->mAngleInnerCone),
                .outerConeAngle = glm::degrees(assimpLight->mAngleOuterCone),
                .attenuation = assimpLight->mAttenuationQuadratic
            };
            ecs::get<Position>(lightEntity).position = glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z};
            ecs::get<Direction>(lightEntity).dir = glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z};
            lights.insert(lightEntity);
        } else if(assimpLight->mType == aiLightSource_AREA) {
            ecs::Entity_t lightEntity = ecs::makeEntity<Light, AreaLight, Position, Direction>();
            ecs::get<Light>(lightEntity) = {
                .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
            };
            ecs::get<AreaLight>(lightEntity) = {
                .attenuation = assimpLight->mAttenuationQuadratic,
                .size = glm::vec2{assimpLight->mSize.x, assimpLight->mSize.y}
            };
            ecs::get<Position>(lightEntity).position = glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z};
            ecs::get<Direction>(lightEntity).dir = glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z};
            lights.insert(lightEntity);
        }
    }

    return std::make_pair(modelEntity, lights);
}
text::Font &game::LevelParser::createFont(std::filesystem::path atlas, std::filesystem::path metadata)
{
    auto pair = std::make_pair(atlas, metadata);
    if(m_fontCache.find(pair) == m_fontCache.end()) {
        m_fontCache.try_emplace(pair, text::Font{atlas, metadata});
    }
    return m_fontCache.at(pair);
}
void game::LevelParser::addTexture(ecs::Entity_t const &modelEntity, std::filesystem::path const &path, std::string const &type, bool flipTextures)
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

game::Scene game::LevelParser::parseScene(std::filesystem::path const &filepath)
{ // FIXME: good luck reading it. 
    std::ifstream filestream{filepath};
    if(!filestream) {
        m_errorStr = "failed to open file";
    }
    Scene scene{};
    scene.filePath = filepath;
    
    json data = json::parse(filestream);
    if(data.contains("entities")) {
        json jsonentities = data["entities"];
        for(auto entityIter = jsonentities.cbegin(); entityIter != jsonentities.cend(); ++entityIter) {
            json const &jsonentity = entityIter.value();
            ecs::Entity_t entity;
            std::string type = jsonentity.contains("type") && jsonentity.at("type").is_string() ? jsonentity["type"].get<std::string>() : "";
                   if(type == "prop") {
                std::filesystem::path path = jsonentity.contains("path") && jsonentity.at("path").is_string() ? jsonentity["path"].get<std::string>() : "";
                if(path == "") {
                    m_errorStr.append("\nno path specified for prop");
                    continue;
                } 
                bool flipTextures = 
                    jsonentity.contains("flip textures") && jsonentity.at("flip textures").is_boolean() ? 
                    jsonentity["flip textures"].get<bool>() :
                    true;
                bool flipWindingOrder = 
                    jsonentity.contains("flip winding order") && jsonentity.at("flip winding order").is_boolean() ? 
                    jsonentity["flip winding order"].get<bool>() :
                    false;
                std::set<ecs::Entity_t> lights;
                std::tie(entity, lights) = createModel(path, flipWindingOrder, flipTextures);
                scene.containedEntities.insert(lights.begin(), lights.end());
                ecs::addComponent(entity, MaterialProperties{});
                MaterialProperties &materialProperties = ecs::get<MaterialProperties>(entity);

                if(jsonentity.contains("textures")) {
                    json jsontextures = jsonentity["textures"];
                    for(auto textureIter = jsontextures.cbegin(); textureIter != jsontextures.cend(); ++textureIter) {
                        json jsontexture = textureIter.value();
                        std::string type = jsontexture.contains("type") && jsontexture.at("type").is_string() ? jsontexture["type"].get<std::string>() : "diffuse";
                        std::filesystem::path path = jsontexture.contains("path") && jsontexture.at("path").is_string() ? jsontexture["path"].get<std::string>() : "";

                        if(path == "") {
                            m_errorStr.append("\nno path specified for texture");
                            continue;
                        }
                        addTexture(entity, path, type, flipTextures);
                    }
                }
                if(jsonentity.contains("position") && jsonentity.at("position").is_array()) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.contains("rotation") && jsonentity.at("rotation").is_array()) {
                    ecs::addComponent<game::OrientationEuler>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["rotation"]))});
                }
                if(jsonentity.contains("scale") && jsonentity.at("scale").is_array()) {
                    ecs::addComponent<game::Scale>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["scale"]))});
                }
                if(jsonentity.contains("repeat textures") && jsonentity.at("repeat textures").is_number()) {
                    ecs::addComponent(entity, RepeatTexture{jsonentity["repeat textures"].get<unsigned>()});
                }
                if(jsonentity.contains("shininess") && jsonentity.at("shininess").is_number()) {
                    materialProperties.shininess = jsonentity["shininess"].get<float>();
                } else {
                    aiMaterial const *mat = ecs::get<model::Model>(entity).getScene()->mMaterials[0];
                    if(aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &materialProperties.shininess) != AI_SUCCESS) {
                        materialProperties.shininess = 0;
                    }
                }
                if(materialProperties.shininess == 0)  {
                    materialProperties.shininess = 16;
                }
                if(jsonentity.contains("color") && jsonentity.at("color").is_array() && jsonentity.at("color").at(0).is_number()) {
                    Color color;
                    color.color = glm::vec4{0, 0, 0, 1};

                    for(size_t i = 0; i < jsonentity.at("color").size(); ++i) {
                        color.color[i] = jsonentity.at("color").at(i);
                    }
                    ecs::addComponent(entity, color);
                }
                if(
                    (jsonentity.contains("transparent") && jsonentity.at("transparent").is_boolean() && jsonentity.at("transparent").get<bool>()) || 
                    (ecs::entityHasComponent<Color>(entity) && ecs::get<Color>(entity).color.a < 1)
                ) ecs::addComponent<Transparent>(entity);
                if(
                    (jsonentity.contains("semi-transparent") && jsonentity.at("semi-transparent").is_boolean() && jsonentity.at("semi-transparent").get<bool>()) || 
                    (ecs::entityHasComponent<Color>(entity) && ecs::get<Color>(entity).color.a < 1)
                ) ecs::addComponent<SemiTransparent>(entity);
            } else if(type == "controllable camera") {
                GLFWwindow *window = findWindow();
                if(!window) {
                    m_errorStr.append("\nwindow not found for controllable camera");
                    continue;
                }
                entity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Window>();
                ecs::get<ControllableCamera>(entity) = {
                    .speedUnitsPerSecond = jsonentity.contains("speed") && jsonentity.at("speed").is_number() ? jsonentity["speed"].get<float>() : 1,
                    .sensitivity = 0.1,
                    .locked = true
                };
                ecs::get<RenderTarget>(entity) = {};
                ecs::get<RenderTarget>(entity).clearColor = {0.2, 0.2, 0.2, 1};
                ecs::get<Camera>(entity) = {};
                ecs::get<Window>(entity) = {window};

                if(jsonentity.contains("position")) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.contains("rotation")) {
                    ecs::addComponent<game::OrientationEuler>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["rotation"]))});
                }
            } else if(type == "camera") {
                entity = ecs::makeEntity<Camera, PerspectiveProjection, RenderTarget>();
                ecs::get<RenderTarget>(entity) = {};
                ecs::get<RenderTarget>(entity).clearColor = {0.2, 0.2, 0.2, 1};
                ecs::get<Camera>(entity) = {};

                if(jsonentity.contains("position")) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.contains("rotation")) {
                    ecs::addComponent<game::OrientationEuler>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["rotation"]))});
                }
            } else if(type == "point light") {
                entity = ecs::makeEntity<Light, PointLight>();
                ecs::get<Light>(entity) = {
                    .color = jsonentity.contains("color") && jsonentity.at("color").is_array() ? static_cast<glm::vec3>(getVecFromJSON(jsonentity["color"])) : glm::vec3{1},
                };
                ecs::get<PointLight>(entity) = {
                    .attenuation = jsonentity.contains("attenuation") && jsonentity.at("attenuation").is_number() ? jsonentity.at("attenuation").get<float>() : 10.0f
                };
                if(jsonentity.contains("position")) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
            } else if(type == "directional light") {
                entity = ecs::makeEntity<game::Light, game::DirectionalLight>();
                ecs::get<Light>(entity) = {
                    .color = jsonentity.contains("color") && jsonentity.at("color").is_array() ? static_cast<glm::vec3>(getVecFromJSON(jsonentity["color"])) : glm::vec3{1},
                };
                // ecs::get<DirectionalLight>(entity) = {};
                if(jsonentity.contains("direction")) {
                    ecs::addComponent<game::Direction>(entity, {glm::normalize(static_cast<glm::vec3>(getVecFromJSON(jsonentity["direction"])))});
                }
            } else if(type == "spot light") {
                entity = ecs::makeEntity<game::Light, game::SpotLight>();
                ecs::get<Light>(entity) = {
                    .color = jsonentity.contains("color") && jsonentity.at("color").is_array() ? static_cast<glm::vec3>(getVecFromJSON(jsonentity["color"])) : glm::vec3{1},
                };
                ecs::get<SpotLight>(entity) = {
                    .innerConeAngle = jsonentity.contains("inner cone angle") && jsonentity.at("inner cone angle").is_number() ? jsonentity.at("inner cone angle").get<float>() : 35.0f,
                    .outerConeAngle = jsonentity.contains("outer cone angle") && jsonentity.at("outer cone angle").is_number() ? jsonentity.at("outer cone angle").get<float>() : 45.0f,
                    .attenuation = jsonentity.contains("attenuation") && jsonentity.at("attenuation").is_number() ? jsonentity.at("attenuation").get<float>() : 10.0f
                };
                if(jsonentity.contains("position")) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.contains("direction")) {
                    ecs::addComponent<game::Direction>(entity, {glm::normalize(static_cast<glm::vec3>(getVecFromJSON(jsonentity["direction"])))});
                }
            } else if(type == "area light") {
                entity = ecs::makeEntity<game::Light, game::AreaLight>();
                ecs::get<Light>(entity) = {
                    .color = jsonentity.contains("color") && jsonentity.at("color").is_array() ? static_cast<glm::vec3>(getVecFromJSON(jsonentity["color"])) : glm::vec3{1},
                };
                ecs::get<AreaLight>(entity) = {
                    .attenuation = jsonentity.contains("attenuation") && jsonentity.at("attenuation").is_number() ? jsonentity.at("attenuation").get<float>() : 10.0f,
                    .size = jsonentity.contains("size") && jsonentity.at("size").is_array() ? getVecFromJSON<2>(jsonentity.at("size")) : glm::vec2{1}
                };
                if(jsonentity.contains("position")) {
                    ecs::addComponent<game::Position>(entity, {static_cast<glm::vec3>(getVecFromJSON(jsonentity["position"]))});
                }
                if(jsonentity.contains("direction")) {
                    ecs::addComponent<game::Direction>(entity, {glm::normalize(static_cast<glm::vec3>(getVecFromJSON(jsonentity["direction"])))});
                }
            } else if(type == "text") {
                entity = ecs::makeEntity<game::Text>();
                assert(jsonentity.contains("font") && jsonentity.at("font").is_object());
                assert(jsonentity["font"].contains("atlas") && jsonentity["font"].at("atlas").is_string());
                assert(jsonentity["font"].contains("metadata") && jsonentity["font"].at("metadata").is_string());
                std::string text = jsonentity.contains("string") && jsonentity.at("string").is_string() ? jsonentity["string"].get<std::string>() : "text";
                glm::vec2 position = jsonentity.contains("position") && jsonentity.at("position").is_array() ? getVecFromJSON<2>(jsonentity["position"]) : glm::vec2{0};
                float size = jsonentity.contains("size") && jsonentity.at("size").is_number() ? jsonentity["size"].get<float>() : 1;
                glm::vec4 fgColor = jsonentity.contains("foreground color") && jsonentity.at("foreground color").is_array() ? getVecFromJSON<4>(jsonentity["foreground color"]) : glm::vec4{1};
                glm::vec4 bgColor = jsonentity.contains("background color") && jsonentity.at("background color").is_array() ? getVecFromJSON<4>(jsonentity["background color"]) : glm::vec4{0};
                
                ecs::get<Text>(entity) = Text{
                    .font = &createFont(jsonentity["font"]["atlas"].get<std::string>(), jsonentity["font"]["metadata"].get<std::string>()),
                    .text = text,
                    .position = position,
                    .size = size,
                    .fgColor = fgColor,
                    .bgColor = bgColor
                };
            } else {
                m_errorStr.append("warn: unrecognized type: " + type);
            }
            scene.containedEntities.insert(entity);
        }
    }
    return scene;
}
