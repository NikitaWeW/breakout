#include "controller.hpp"
#include "engine/DSA/ECS.hpp"
#include "engine/Header/Config.hpp"
#include "engine/Logging/Logging.hpp"
#include "engine/Renderer/EngineRenderer.hpp"
#include "engine/Resource/Loaders.hpp"
#include "scene.hpp"
#include <fstream>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nicecs/ecs.hpp>

using namespace engine;
using namespace nlohmann;

static const Scene INVALID_SCENE = {};

static glm::vec4 hexToRgba(std::string hex)
{
    if(!hex.empty() && hex[0] == '#')
        hex.erase(0, 1);

    if(hex.size() == 6)
    {
        int r, g, b;
        std::sscanf(hex.c_str(), "%02x%02x%02x", &r, &g, &b);
        return glm::vec4(r, g, b, 1) / 255.0f;
    } else if(hex.size() == 8)
    {
        int r, g, b, a;
        std::sscanf(hex.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a);
        return glm::vec4(r, g, b, a) / 255.0f;
    }
    else {
        ENGINE_WARN("Invalid hex: \"#{}\"", hex);
        return glm::vec4(0, 0, 0, 1);
    }
}
static glm::vec2 toVec2(json const &json)
{
    ENGINE_ASSERT_MSG(json.is_array() && json.size() == 2, "Invalid vec2 json!");

    return glm::vec2(json[0].get<float>(), json[1].get<float>());
}
static glm::vec3 toVec3(json const &json)
{
    if(json.is_array())
    {
        ENGINE_ASSERT(json.size() == 3);
        return glm::vec3(json.at(0).get<float>(), json.at(1).get<float>(), json.at(2).get<float>());
    } else if(json.is_string())
    {
        return glm::vec3(hexToRgba(json.get<std::string>()));
    }

    ENGINE_ASSERT_MSG(false, "Object is not array nor a hex string");
}
static glm::vec4 toVec4(json const &json)
{
    if(json.is_array())
    {
        ENGINE_ASSERT(json.size() == 4);
        return glm::vec4(json.at(0).get<float>(), json.at(1).get<float>(), json.at(2).get<float>(), json.at(3).get<float>());
    } else if(json.is_string())
    {
        return glm::vec4(hexToRgba(json.get<std::string>()));
    }

    ENGINE_ASSERT_MSG(false, "Object is not array nor a hex string");
}
/* 
static void processTags(json const &json, Entity &entity)
{
    if(json.contains("tags"))
    {
        auto const &jTags = json["tags"];
        entity.emplace<Tags>();
        auto &tags = entity.get<Tags>();
        for(auto const &tag : jTags)
            tags.tags.emplace_back(tag.get<std::string>());
    }
}
*/

struct ModelResult
{
    Entity instance;
    Entity model;
};
struct SceneLoaderImpl
{
    nlohmann::json mSchema;
    nlohmann::json_schema::json_validator mSchemaValidator;
    engine::Registry *mReg = nullptr;
    engine::ModelLoader mModelLoader;
    engine::TextureLoader mTextureLoader;
    engine::CubemapLoader mCubemapLoader;

    SceneLoaderImpl() = default;
    SceneLoaderImpl(Registry &reg, std::string_view schemaPath);

    void processScene(Scene &scene, engine::Registry &reg);
    Scene load(std::string_view path);
    ecs::entity loadTexture(json const &textures, std::string_view key, ecs::entity defaultTexture);
    ModelResult processModel(json const &jModel);
    Entity processLight(json const &jLight);
};

ecs::entity SceneLoaderImpl::loadTexture(json const &textures, std::string_view key, ecs::entity defaultTexture)
{
    return textures.contains(key) ? mTextureLoader.loadFromFile(textures[key].get<std::string>()) : defaultTexture;
}

SceneLoaderImpl::SceneLoaderImpl(Registry &reg, std::string_view schemaPath) : mReg(&reg), mModelLoader(reg), mTextureLoader(reg), mCubemapLoader(reg)
{
    std::ifstream file(std::string{schemaPath});
    if(!file.is_open())
    {
        ENGINE_ERROR("Failed to open schema file \"{}\"!", schemaPath);
        mReg = nullptr; // invalidate loader
    }
    mSchema = json::parse(file);

    try {
        mSchemaValidator.set_root_schema(mSchema);
    } catch (const std::exception &e) {
        ENGINE_ERROR("Validation of schema failed: {}", e.what());
        mReg = nullptr;
    }
}
SceneLoader::SceneLoader(Registry &reg, std::string_view schemaPath) : Handle(new SceneLoaderImpl{reg, schemaPath}) {}

ModelResult SceneLoaderImpl::processModel(json const &jModel)
{
    auto modelPath = jModel["path"].get<std::string>();
    auto model = mModelLoader.loadFromFile(modelPath);
    if(model == INVALID_ENTITY)
    {
        return {};
    }
    Entity eModel{*mReg};
    eModel.emplace<Instance>(Instance{
        .e_model = model
    });

    if(jModel.contains("transform"))
    {
        auto const &jTransform = jModel["transform"];
        Transform transform;
        if(jTransform.contains("position"))
            transform.position = toVec3(jTransform["position"]);
        if(jTransform.contains("rotation"))
            transform.orientation = glm::quat(toVec3(jTransform["rotation"]));
        if(jTransform.contains("scale"))
            transform.scale = toVec3(jTransform["scale"]);
        eModel.emplace<Transform>(std::move(transform));
    }

    if(jModel.contains("material"))
    {
        auto const &textures = jModel["material"]["textures"];
        auto const &properties = jModel["material"]["properties"];
        auto const &defaultTextures = mModelLoader.getDefaultMaterial().textures;
        auto const &defaultProperties = mModelLoader.getDefaultMaterial().properties;
        eModel.emplace<Material>(Material{
            .textures = {
                .albedo       = loadTexture(textures, "albedo",       defaultTextures.albedo),
                .metallic     = loadTexture(textures, "metallic",     defaultTextures.metallic),
                .roughness    = loadTexture(textures, "roughness",    defaultTextures.roughness),
                .ambient      = loadTexture(textures, "ambient",      defaultTextures.ambient),
                .normal       = loadTexture(textures, "normal",       defaultTextures.normal),
                .displacement = loadTexture(textures, "displacement", defaultTextures.displacement),
                .alpha        = loadTexture(textures, "alpha",        defaultTextures.alpha),
            },
            .properties = {
                .ambient = properties.contains("ambient") ? toVec3(properties["ambient"]) : defaultProperties.ambient,
                .albedo = properties.contains("albedo") ? toVec4(properties["albedo"]) : defaultProperties.albedo,
                .specular = properties.contains("specular") ? toVec3(properties["specular"]) : defaultProperties.specular,
                .emission = properties.contains("emission") ? toVec3(properties["emission"]) : defaultProperties.albedo,
                .shininess = properties.contains("shininess") ? properties["shininess"].get<float>() : defaultProperties.shininess,
                .metallic = properties.contains("metallic") ? properties["metallic"].get<float>() : defaultProperties.metallic,
                .ior = properties.contains("ior") ? properties["ior"].get<float>() : defaultProperties.ior,
            }
        });
        if(eModel.get<Material>().properties.albedo.a < 1.0f)
            eModel.emplace<Transparent>();
    }

    return { eModel, {*mReg, model} };
}

static AreaLight::Shape toAreaLightShape(std::string_view shape)
{
    if(shape == "rectangle") return AreaLight::Shape::RECTANGLE;
    else if(shape == "circle") return AreaLight::Shape::CIRCLE;
    ENGINE_ASSERT_MSG(false, "Unknown area light shape!");
}

Entity SceneLoaderImpl::processLight(json const &jLight)
{
    auto type = jLight["type"].get<std::string>();
    glm::vec3 color = toVec3(jLight["color"]) * jLight["intensity"].get<float>();
    Entity eLight{*mReg};

    if(type == "point")
    {
        eLight.emplace<PointLight>(PointLight{
            .color = color
        });
    } else if(type == "directional")
    {
        eLight.emplace<DirectionalLight>(DirectionalLight{
            .color = color
        });
    } else if(type == "spot")
    {
        if(!jLight.contains("in out cones"))
        {
            ENGINE_ERROR("Spot light doesent contain \"in out cones\"!");
            eLight.reg().destroy(eLight.entity());
            return {};
        }
        eLight.emplace<SpotLight>(SpotLight{
            .color = color,
            .innerConeAngle = jLight["in out cones"][0].get<float>(),
            .outerConeAngle = jLight["in out cones"][1].get<float>(),
        });
    } else if(type == "area")
    {
        if(!jLight.contains("size") && !jLight.contains("shape"))
        {
            ENGINE_ERROR("Area light doesent contain \"size\" or \"shape\"!");
            eLight.reg().destroy(eLight.entity());
            return {};
        }
        eLight.emplace<AreaLight>(AreaLight{
            .color = color,
            .size = toVec2(jLight["size"]),
            .shape = toAreaLightShape(jLight["shape"].get<std::string>()),
        });
    } else {
        ENGINE_ERROR("Unknown light type: \"{}\"", type);
        eLight.reg().destroy(eLight.entity());
        return {};
    }

    if(jLight.contains("position") || jLight.contains("direction"))
    {
        Transform transform;
        if(jLight.contains("position"))
            transform.position = toVec3(jLight["position"]);
        if(jLight.contains("direction"))
        {
            auto dir = glm::normalize(toVec3(jLight["direction"]));
            transform.orientation = glm::quatLookAt(dir, glm::abs(glm::dot(dir, glm::vec3(0,1,0))) > 0.999 ? glm::vec3(1,0,0) : glm::vec3(0,1,0));
        }
            
        eLight.emplace<Transform>(std::move(transform));
    }

    if(jLight.contains("shadow"))
    {
        auto const &shadow = jLight["shadow"];
        eLight.emplace<ShadowLight>(ShadowLight{
             .shadowMapSize = shadow["sm size"].get<unsigned>(),
            .nearPlane = shadow["znear"].get<float>(),
            .farPlane = shadow["zfar"].get<float>(),
        });
    }

    eLight.emplace<Version>();

    return eLight;
}


void SceneLoaderImpl::processScene(Scene &scene, Registry &reg)
{
    auto const &root = scene.data;
    scene.name = root["name"].get<std::string>();

    for(auto &modelEntry : root["models"])
    {
        auto [eModel, model] = processModel(modelEntry);
        if(!eModel.valid() || !model.valid())
        {
            ENGINE_ERROR("Failed to load model \"{}\" from scene \"{}\"", modelEntry["path"].get<std::string>(), scene.path);
            // Might add a bit of error handling here.
            continue;
        }

        scene.entities.emplace_back(eModel);
        scene.models.emplace(eModel);
        auto const &lights = mReg->get<Model>(eModel.get<Instance>().e_model).lights;
        for(auto const &eLight : lights)
            scene.entities.emplace_back(*mReg, eLight);
    }

    if(root.contains("lights"))
    {
        for(auto &lightEntry : root["lights"])
        {
            auto eLight = processLight(lightEntry);
            scene.entities.emplace_back(eLight);
        }
    }

    if(root.contains("skybox"))
    {
        scene.entities.emplace_back(*mReg, mReg->create(Skybox{
            .e_cubemap = mCubemapLoader.loadFromFile(root["skybox"].get<std::string>(), {.flip = false})
        }));
    }

    scene.valid = true;
}

Scene SceneLoaderImpl::load(std::string_view path)
{
    if(!mReg)
    {
        ENGINE_ERROR("Failed to load scene \"{}\": Invalid SceneLoader!", path);
        return INVALID_SCENE;
    }
    std::ifstream file(std::string{path});
    if(!file.is_open())
    {
        ENGINE_ERROR("Failed to open scene file \"{}\"", path);
        return INVALID_SCENE;
    }
    Scene scene;
    scene.path = path;
    try {
        scene.data = json::parse(file, nullptr, true, /* Ignore comments (jsonc) */ true);
    } catch(json::parse_error &e)
    {
        ENGINE_ERROR("Failed to parse scene file \"{}\": {}", path, e.what());
        return INVALID_SCENE;
    }

    try {
        mSchemaValidator.validate(scene.data);
    } catch (const std::exception &e) {
        ENGINE_ERROR("Scene file \"{}\" validation failed: ", path, e.what());
        return INVALID_SCENE;
    }

    processScene(scene, *mReg);

    return scene;
}
Scene SceneLoader::load(std::string_view path)
{
    if(this->empty())
    {
        ENGINE_ERROR("Failed to load scene \"{}\": Invalid SceneLoaderImpl (default constructed)!", path);
        return INVALID_SCENE;
    }
    return unwrap().load(path);
}
