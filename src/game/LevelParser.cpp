#include "LevelParser.hpp"
#include "CommonTypes.hpp"
#include "Controller.hpp"
#include "Animator.hpp"
#include <iostream>
using json = nlohmann::json;

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
template <typename T, typename IsSomethingFunc_t>
T get(json const &j, std::string_view key, T const &def = T{}, IsSomethingFunc_t isSomething = nullptr)
{
    // bool isTheThingTheThingThatTheThingIsAndIsTheThingTheThingThatIsContainingTheThingThatTheThingContains = j.contains(key) && (!isSomething || (j.at(key).*isSomething)());
    return j.contains(key) && (!isSomething || (j.at(key).*isSomething)()) ? j.at(key).get<T>() : def;
}
template <size_t L = 3>
glm::vec<L, float> get(json const &j, std::string_view key, glm::vec<L, float> const &def = glm::vec<L, float>{0})
{
    return j.contains(key) && j.at(key).is_array() && j.at(key).at(0).is_number() ? getVecFromJSON<L>(j.at(key)) : def;
}
template <size_t L = 3>
glm::vec<L, float> getColor(json const &j, std::string_view key = "color", glm::vec<L, float> const &def = glm::vec<L, float>{1})
{
    if(j.contains(key) && j.at(key).is_array() && j.at(key).at(0).is_number()) {
        glm::vec<L, float> color;
        for(size_t i = 0; i < L && i < j.at(key).size(); ++i) {
            color[i] = j.at(key).at(i);
        }
        return color;
    } else if(j.contains(key) && j.at(key).is_string()) {
        std::string hex = j.at(key).get<std::string>();
        if(hex.at(0) == '#') {
            hex.erase(0, 1);
        }
        
        constexpr unsigned splitLength = 2;
        int NumSubstrings = hex.length() / splitLength;
        std::vector<std::string> splittedStrings;

        for (int i = 0; i < NumSubstrings; i++) {
            splittedStrings.emplace_back(hex.substr(i * splitLength, splitLength));
        }

        // If there are leftover characters, create a shorter item at the end.
        if (hex.length() % splitLength != 0) {
            splittedStrings.emplace_back(hex.substr(splitLength * NumSubstrings));
        }

        glm::vec<L, float>color{1};
        for(size_t i = 0; i < L && i < splittedStrings.size(); ++i) {
            color[i] = stoi(splittedStrings[i], nullptr, 16) / 255.0f;
        }

        return color;
    } else {
        return def;
    }
}
GLFWwindow *findWindow() 
{
    auto iter = std::find_if(
        ecs::getSystemManager().getEntities().cbegin(), 
        ecs::getSystemManager().getEntities().cend(), 
        [](ecs::Entity_t const &entity){ 
            return ecs::has<game::Window>(entity); 
        }
    );
    if(iter == ecs::getSystemManager().getEntities().cend()) {
        return nullptr;
    } else {
        return ecs::get<game::Window>(*iter).glfwwindow;
    }
}

class PropCreator : public game::ILevelEntityCreator
{
private:
    std::map<std::filesystem::path, opengl::Texture> m_textureCache;
    std::map<std::filesystem::path, model::Model> m_modelCache;

    void addTexture(ecs::Entity_t const &modelEntity, std::filesystem::path const &path, std::string const &type, bool flipTextures)
    {
        using namespace game;
        if(!ecs::has<model::Model>(modelEntity)) return;
        model::Model &model = ecs::get<model::Model>(modelEntity);

        if(m_textureCache.find(path) == m_textureCache.end()) {
            m_textureCache.try_emplace(path, path, flipTextures, type == "diffuse", type);
        }
        opengl::Texture &texture = m_textureCache.at(path);
        texture.type = type;
        for(model::Mesh &mesh : model.getMeshes()) {
            mesh.textures.push_back(texture);
        }
    }
    std::pair<ecs::Entity_t, std::set<ecs::Entity_t>> createModel(std::filesystem::path const &filepath, bool flipWindingOrder, bool flipTextures)
    {
        using namespace game;
        if(m_modelCache.find(filepath) == m_modelCache.end()) {
            m_modelCache.try_emplace(filepath, 
                filepath, 
                model::FLIP_TEXTURES | 
                model::LOAD_DRAWABLE | 
                (flipWindingOrder ? model::FLIP_WINDING_ORDER : 0) | 
                (flipTextures ? model::FLIP_TEXTURES : 0));
        }
        model::Model const &model = m_modelCache.at(filepath);

        ecs::Entity_t modelEntity = ecs::makeEntity<model::Model>();
        ecs::get<model::Model>(modelEntity) = model;

        if(model.getScene()->HasAnimations()) {
            game::Animation animation;

            ecs::add<game::Animation>(modelEntity);
            ecs::get<game::Animation>(modelEntity) = animation;
        }
        std::set<ecs::Entity_t> lights;
        for(size_t i = 0; i < model.getScene()->mNumLights; ++i) {
            aiLight const *assimpLight = model.getScene()->mLights[i];
            
            if(assimpLight->mType == aiLightSource_POINT) {
                ecs::Entity_t lightEntity = ecs::makeEntity<Light, PointLight, Position>();
                ecs::get<Light>(lightEntity) = {
                    .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
                };
                ecs::get<PointLight>(lightEntity) = {
                    .attenuation = assimpLight->mAttenuationQuadratic
                };
                ecs::get<Position>(lightEntity) = { glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z} };
                lights.insert(lightEntity);
            } else if(assimpLight->mType == aiLightSource_DIRECTIONAL) {
                ecs::Entity_t lightEntity = ecs::makeEntity<Light, DirectionalLight, Direction>();
                ecs::get<Light>(lightEntity) = {
                    .color = glm::vec3{assimpLight->mColorDiffuse.r, assimpLight->mColorDiffuse.g, assimpLight->mColorDiffuse.b}
                };
                ecs::get<Direction>(lightEntity) = { glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z} };
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
                ecs::get<Position>(lightEntity) = { glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z} };
                ecs::get<Direction>(lightEntity) = { glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z} };
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
                ecs::get<Position>(lightEntity) = { glm::vec3{assimpLight->mPosition.x, assimpLight->mPosition.y, assimpLight->mPosition.z} };
                ecs::get<Direction>(lightEntity) = { glm::vec3{assimpLight->mDirection.x, assimpLight->mDirection.y, assimpLight->mDirection.z} };
                lights.insert(lightEntity);
            }
        }

        return std::make_pair(modelEntity, lights);
    }
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        std::filesystem::path path = get<std::string>(jsonentity, "path", "", &json::is_string);
        if(path == "") {
            std::cout << "no path specified for prop!\n";
            return;
        } 
        bool flipTextures = get(jsonentity, "flip textures", true, &json::is_boolean);
        bool flipWindingOrder = get(jsonentity, "flip winding order", false, &json::is_boolean);
        auto [entity, lights] = createModel(path, flipWindingOrder, flipTextures);
        scene.containedEntities.insert(lights.begin(), lights.end());
        ecs::add(entity, MaterialProperties{});
        MaterialProperties &materialProperties = ecs::get<MaterialProperties>(entity);

        if(jsonentity.contains("textures")) {
            json jsontextures = jsonentity["textures"];
            for(auto textureIter = jsontextures.cbegin(); textureIter != jsontextures.cend(); ++textureIter) {
                json jsontexture = textureIter.value();
                std::string type = get<std::string>(jsontexture, "type", "diffuse", &json::is_string);
                std::filesystem::path texturePath = get<std::string>(jsontexture, "path", "", &json::is_string);

                if(texturePath == "") {
                    std::cout << "no path specified for texture!\n";
                    return;
                }
                addTexture(entity, texturePath, type, flipTextures);
            }
        }

        ecs::add<game::Position>(entity, {get(jsonentity, "position")});
        ecs::add<game::OrientationEuler>(entity, {get(jsonentity, "rotation")});
        ecs::add<game::Scale>(entity, {get<3>(jsonentity, "scale", glm::vec3{1})});
        if(get<bool>(jsonentity, "casts shadow", true, &json::is_boolean)) 
            ecs::add<CastsShadow>(entity);
        if(jsonentity.contains("repeat textures") && jsonentity.at("repeat textures").is_number()) {
            ecs::add(entity, RepeatTexture{jsonentity["repeat textures"].get<unsigned>()});
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
        ecs::add(entity, Color{
            .color = getColor<4>(jsonentity)
        });
        if(
            (jsonentity.contains("transparent") && jsonentity.at("transparent").is_boolean() && jsonentity.at("transparent").get<bool>()) || 
            (ecs::has<Color>(entity) && ecs::get<Color>(entity).color.a < 1)
        ) ecs::add<Transparent>(entity);
        if(jsonentity.contains("semi-transparent") && jsonentity.at("semi-transparent").is_boolean() && jsonentity.at("semi-transparent").get<bool>()) 
            ecs::add<SemiTransparent>(entity);

        scene.containedEntities.insert(entity);
    }
};
class ControllableCameraCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        GLFWwindow *window = findWindow();
        if(!window) {
            std::cout << "window not found for controllable camera!\n";
            return;
        }
        ecs::Entity_t entity = ecs::makeEntity<Camera, PerspectiveProjection, ControllableCamera, RenderTarget, Window>();
        ecs::get<ControllableCamera>(entity) = {
            .speedUnitsPerSecond = get<float>(jsonentity, "speed", 1.0f, &json::is_number),
            .sensitivity = get<float>(jsonentity, "sensitivity", 0.1f, &json::is_number),
            .locked = true
        };
        ecs::get<RenderTarget>(entity) = {};
        ecs::get<RenderTarget>(entity).clearColor = {0, 0, 0, 1};
        ecs::get<Camera>(entity) = {};
        ecs::get<Window>(entity) = {window};

        ecs::add<game::Position>(entity, {get(jsonentity, "position")});
        ecs::add<game::OrientationEuler>(entity, {get(jsonentity, "rotation")});
        scene.containedEntities.insert(entity);
    }
};
class CameraCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        ecs::Entity_t entity = ecs::makeEntity<Camera, PerspectiveProjection, RenderTarget>();
        ecs::get<RenderTarget>(entity) = {};
        ecs::get<RenderTarget>(entity).clearColor = {0, 0, 0, 1};
        ecs::get<Camera>(entity) = {};

        if(jsonentity.contains("position")) {
            ecs::add<game::Position>(entity, {getVecFromJSON(jsonentity["position"])});
        }
        if(jsonentity.contains("rotation")) {
            ecs::add<game::OrientationEuler>(entity, {getVecFromJSON(jsonentity["rotation"])});
        }
        scene.containedEntities.insert(entity);
    }
};
class PointLightCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        ecs::Entity_t entity = ecs::makeEntity<Light, PointLight>();
        ecs::get<Light>(entity) = {
            .color = getColor<3>(jsonentity)
        };
        ecs::get<PointLight>(entity) = {
            .attenuation = get<float>(jsonentity, "attenuation", 10.0f, &json::is_number)
        };
        ecs::add<game::Position>(entity, {get(jsonentity, "position")});
        if(get<bool>(jsonentity, "casts shadow", true, &json::is_boolean)) 
            ecs::add<ShadowCaster>(entity);
        scene.containedEntities.insert(entity);
    }
};
class DirLightCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        ecs::Entity_t entity = ecs::makeEntity<game::Light, game::DirectionalLight>();
        ecs::get<Light>(entity) = {
            .color = getColor<4>(jsonentity)
        };
        ecs::add<game::Direction>(entity, {get<3>(jsonentity, "direction", glm::vec3{1, 0, 0})});
        if(get<bool>(jsonentity, "does shadow", true, &json::is_boolean)) ecs::add<ShadowCaster>(entity);
        scene.containedEntities.insert(entity);
    }
};
class SpotLightCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        ecs::Entity_t entity = ecs::makeEntity<game::Light, game::SpotLight>();
        ecs::get<Light>(entity) = {
            .color = getColor<4>(jsonentity)
        };
        ecs::get<SpotLight>(entity) = {
            .innerConeAngle = get<float>(jsonentity, "inner cone angle", 35.0f, &json::is_number),
            .outerConeAngle = get<float>(jsonentity, "outer cone angle", 45.0f, &json::is_number),
            .attenuation = get<float>(jsonentity, "attenuation", 10.0f, &json::is_number) 
        };
        ecs::add<game::Position>(entity, {get<3>(jsonentity, "position")});
        ecs::add<game::Direction>(entity, {get<3>(jsonentity, "direction", glm::vec3{1, 0, 0})});
        if(get<bool>(jsonentity, "casts shadow", true, &json::is_boolean)) 
            ecs::add<ShadowCaster>(entity);
        scene.containedEntities.insert(entity);
    }
};
class AreaLightCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        using namespace game;
        ecs::Entity_t entity = ecs::makeEntity<game::Light, game::AreaLight>();
        ecs::get<Light>(entity) = {
            .color = getColor<4>(jsonentity)
        };
        ecs::get<AreaLight>(entity) = {
            .attenuation = get<float>(jsonentity, "attenuation", 10.0f, &json::is_number),
            .size = get<2>(jsonentity, "size", glm::vec2{1})
        };
        ecs::add<game::Position>(entity, {get<3>(jsonentity, "position")});
        ecs::add<game::Direction>(entity, {get<3>(jsonentity, "direction", glm::vec3{1, 0, 0})});
        if(get<bool>(jsonentity, "casts shadow", true, &json::is_boolean)) 
            ecs::add<ShadowCaster>(entity);
        scene.containedEntities.insert(entity);
    }
};
class TextCreator : public game::ILevelEntityCreator
{
private:
    std::map<std::pair<std::filesystem::path, std::filesystem::path>, text::Font> m_fontCache;
    // pair(model entity, set of light entities)
    text::Font &createFont(std::filesystem::path atlas, std::filesystem::path metadata)
    {
        auto pair = std::make_pair(atlas, metadata);
        if(m_fontCache.find(pair) == m_fontCache.end()) {
            m_fontCache.try_emplace(pair, atlas, metadata);
        }
        return m_fontCache.at(pair);
    }
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        ecs::Entity_t entity = ecs::makeEntity<game::Text>();
        assert(jsonentity.contains("font") && jsonentity.at("font").is_object());
        std::string text = get<std::string>(jsonentity, "string", "lorem ispum", &json::is_string);
        glm::vec2 position = get<2>(jsonentity, "position", glm::vec2{0});
        float size = get<float>(jsonentity, "size", 1.0f, &json::is_number);
        glm::vec4 fgColor = get<4>(jsonentity, "foreground color", glm::vec4{1});
        glm::vec4 bgColor = get<4>(jsonentity, "background color", glm::vec4{0});
        
        ecs::get<game::Text>(entity) = game::Text{
            .font = &createFont(jsonentity["font"]["atlas"].get<std::string>(), jsonentity["font"]["metadata"].get<std::string>()),
            .text = text,
            .position = position,
            .size = size,
            .fgColor = fgColor,
            .bgColor = bgColor
        };
        scene.containedEntities.insert(entity);
    }
};
class SkyboxCreator : public game::ILevelEntityCreator
{
public:
    void create(json const &jsonentity, game::Scene &scene) override
    {
        std::filesystem::path path = get<std::string>(jsonentity, "path", "", &json::is_string);
        if(path == "") {
            std::cout << "no path specified for skybox!\n";
            return;
        } 
        bool flip = get<bool>(jsonentity, "flip", false, &json::is_boolean);
        ecs::Entity_t entity = ecs::makeEntity<opengl::Cubemap, game::Skybox>();
        ecs::get<opengl::Cubemap>(entity) = opengl::Cubemap{path, flip};
        scene.containedEntities.insert(entity);
    }
};

game::LevelParser::LevelParser()
{
    registerCreator("prop",                std::make_unique<PropCreator>());
    registerCreator("controllable camera", std::make_unique<ControllableCameraCreator>());
    registerCreator("camera",              std::make_unique<CameraCreator>());
    registerCreator("point light",         std::make_unique<PointLightCreator>());
    registerCreator("directional light",   std::make_unique<DirLightCreator>());
    registerCreator("spot light",          std::make_unique<SpotLightCreator>());
    registerCreator("area light",          std::make_unique<AreaLightCreator>());
    registerCreator("text",                std::make_unique<TextCreator>());
    registerCreator("skybox",              std::make_unique<SkyboxCreator>());
}

game::LevelParser::~LevelParser() = default;

game::Scene game::LevelParser::parseScene(std::filesystem::path const &filepath)
{

    std::ifstream filestream{filepath};
    if(!filestream) {
        std::cout << "failed to open file \"" << filepath << "\"!\n";
        assert(false);
    }
    Scene scene{};
    scene.filePath = filepath;
    
    json data = json::parse(filestream);
    if(data.contains("entities")) {
        json jsonentities = data["entities"];
        for(auto entityIter = jsonentities.cbegin(); entityIter != jsonentities.cend(); ++entityIter) {
            json const &jsonentity = entityIter.value();
            std::string type = get<std::string>(jsonentity, "type", "unknown", &json::is_string);

            if(type == "unknown") {
                std::cout << "warn: unrecognized type: " << type << '\n';
                continue;
            } 
            auto it = m_creators.find(type);
            if(it == m_creators.begin()) {
                std::cout << "creator not found for type \"" << type << "\"!\n";
                continue;
            }
            it->second->create(jsonentity, scene);
        }
    }
    return scene;
}
