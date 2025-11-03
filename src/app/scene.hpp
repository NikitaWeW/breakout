#pragma once
#include "engine/engine.hpp"
#include "controller.hpp"

struct ChangeAnimationsTag {};

inline std::string printTexture(ecs::entity e_texture, ecs::registry const &reg)
{
    std::stringstream ss;
    auto const &texture = reg.get<engine::Texture>(e_texture);
    ss 
        << 'e' << e_texture << ", \"" 
        << texture.path << "\", \t" 
        << texture.data.getDimensions() << ", \t" 
        << (texture.srgb ? "srgb" : "not srgb");
    return ss.str();
}
inline void printModelData(ecs::entity e_model, ecs::registry const &registry)
{
    ENGINE_ASSERT(registry.has<engine::Model>(e_model));
    engine::Model const &model = registry.get<engine::Model>(e_model);
    ENGINE_INFO("");
    ENGINE_INFO("Model: \"{}\"", model.path);
    ENGINE_INFO("Skeleton: ");
    ENGINE_INFO("  Bone map size / number of bones: {}", model.skeleton.boneMap.size());
    if(model.skeleton.boneMap.size() <= 30)
        for(auto const &[name, id] : model.skeleton.boneMap)
            ENGINE_INFO("    [\"{}\": {}]", name, id);

    ENGINE_INFO("Animations: {}", model.animations.size());
    for(auto const &animation : model.animations)
    {
        ENGINE_INFO("-----------------");
        ENGINE_INFO("Animation: \"{}\"", animation.name);
        ENGINE_INFO("  Duration: {} ticks, tps: {}", animation.durationTicks, animation.ticksPerSecond);
        ENGINE_INFO("  Bones size: {}", animation.bones.size());
    }

    ENGINE_INFO("Meshes: {}", model.meshes.size());
    for(auto const &mesh : model.meshes)
    {
        ENGINE_INFO("-----------------");

        ENGINE_INFO("Geometry:");
        ENGINE_INFO("  Triangles: {}", mesh.geometry.indices.size() / 3);
        ENGINE_INFO("  Indices:   {}", mesh.geometry.indices.size());
        ENGINE_INFO("  Positions: {}", mesh.geometry.positions.size());
        ENGINE_INFO("  TexCoords: {}", mesh.geometry.texCoords.size());
        ENGINE_INFO("  Normals:   {}", mesh.geometry.normals.size());
        ENGINE_INFO("  Tangents:  {}", mesh.geometry.tangents.size());
        ENGINE_INFO("  BoneIDs:   {}", mesh.geometry.boneIDs.size());
        ENGINE_INFO("  Weights:   {}", mesh.geometry.weights.size());
        
        auto const &material = registry.get<engine::Material>(mesh.e_material);
        ENGINE_INFO("Material:");
        ENGINE_INFO("Textures:");
        ENGINE_INFO("  Albedo:       {}", printTexture(material.textures.albedo, registry));
        ENGINE_INFO("  Metallic:     {}", printTexture(material.textures.metallic, registry));
        ENGINE_INFO("  Roughness:    {}", printTexture(material.textures.roughness, registry));
        ENGINE_INFO("  Ambient:      {}", printTexture(material.textures.ambient, registry));
        ENGINE_INFO("  Normal:       {}", printTexture(material.textures.normal, registry));
        ENGINE_INFO("  Displacement: {}", printTexture(material.textures.displacement, registry));
        ENGINE_INFO("  Alpha:        {}", printTexture(material.textures.alpha, registry));
        ENGINE_INFO("Properties:");
        ENGINE_INFO("  Ambient:       {}", fmt::streamed(material.properties.ambient));
        ENGINE_INFO("  Albedo:        {}", fmt::streamed(material.properties.albedo));
        ENGINE_INFO("  Specular:      {}", fmt::streamed(material.properties.specular));
        ENGINE_INFO("  Emission:      {}", fmt::streamed(material.properties.emission));
        ENGINE_INFO("  Shininess:     {}", material.properties.shininess);
        ENGINE_INFO("  Metallic:      {}", material.properties.metallic);
        ENGINE_INFO("  IOR:           {}", material.properties.ior);
    }
}

inline void createScene(ecs::registry &reg)
{
    engine::Loader loader{reg};
    auto cube =    loader.load(engine::DataType::MODEL, "res/models/cube.obj");
    auto suzanne = loader.load(engine::DataType::MODEL, "res/models/suzanne.obj");

    
    // === === === === === === === ===
    // BIG... STUFF... ?? ...
    // === === === === === === === ===
    { // braces so sections fold nicely in my editor
        reg.create( // plane
            engine::Instance{cube}, 
            engine::Position{{0, 0, -2}},
            engine::Scale{glm::vec3{100, 0, 100}}
        );
        reg.create( // deccer cubes (working flawlessly)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/deccer_cubes/SM_Deccer_Cubes_Textured_Complex.glb")
            }, 
            engine::Position{{-5, 2, 0}},
            engine::Scale{glm::vec3{0.5}}
        );
    }

    // === === === === === === === ===
    // RANDOM STUFF
    // === === === === === === === ===
    {
        reg.create( // some random cube
            engine::Instance{cube}, 
            engine::Position{{1, 1, -2}},
            engine::OrientationEulerXYZ{{-45, 90, 35}}
        );
        reg.create( // some random monkey
            engine::Instance{suzanne},
            engine::Position{{-1, 1, 5}},
            engine::Orientation{glm::normalize(glm::angleAxis(
                -26.0f,
                glm::vec3{1.0f, 2, -4}
            ))},
            engine::Scale{glm::vec3{0.5}}
        );
    }

    // === === === === === === === ===
    // ANIMATION TEST
    // === === === === === === === ===
    {

        reg.create( // fox with cycling animations
            ChangeAnimationsTag{},
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/fox.glb")
            }, 
            engine::Position{{4, 0, -7}},
            engine::Scale{glm::vec3{0.01}},
            engine::CurrentAnimation{
                .name = "Survey"
            }
        ); 
        reg.create( // dancing buddy 0 (dances so hard he accelerates into the sky)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/Silly_Dancing.fbx")
            }, 
            engine::Position{{0, 0, -7}},
            engine::Scale{glm::vec3{0.01}},
            engine::Acceleration{.values = {{engine::UID{}, {0, 0.001, 0}}}},
            engine::Velocity{},
            engine::CurrentAnimation{
                .name = "mixamo.com",
                .speed = 4
            }
        );
        reg.create( // dancing buddy 1
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/Gangnam.fbx")
            }, 
            engine::Position{{-2, 0, -8}},
            engine::Scale{glm::vec3{0.01}},
            engine::CurrentAnimation{
                .name = "mixamo.com",
                .speed = 1
            }
        );
        reg.create( // animation test (made myself thus its so bad)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/blob.glb")
            }, 
            engine::Position{{5, 0, -3}},
            engine::Scale{glm::vec3{0.5}},
            engine::CurrentAnimation{
                .name = "ArmatureAction",
                .speed = 1
            }
        );
    }

    // === === === === === === === ===
    // OIT TEST
    // === === === === === === === ===
    {
        engine::Material material = reg.get<engine::Material>(reg.get<engine::Model>(cube).meshes.at(0).e_material);

        material.properties.albedo = {1, 0.1, 0.1, 0.25};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material)
            },
            engine::Position{{4, 1, 4}},
            engine::Scale{glm::vec3{0, 2, 2}},
            engine::Transparent{}
        );
        material.properties.albedo = {0.1, 1, 0.1, 0.25};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material)
            },
            engine::Position{{5.5, 1, 4}},
            engine::Scale{glm::vec3{0, 2, 2}},
            engine::Transparent{}
        );
        material.properties.albedo = {0.1, 0.1, 1, 0.25};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material)
            },
            engine::Position{{7, 1, 4}},
            engine::Scale{glm::vec3{0, 2, 2}},
            engine::Transparent{}
        );
        material.properties.albedo = {0.9, 0.9, 1, 0.5};
        reg.create(
            engine::Instance{
                suzanne,
                .e_material = reg.create(material)
            },
            engine::Position{{2, 1, 4}},
            engine::Orientation{glm::normalize(glm::angleAxis(
                -54.3f,
                glm::vec3{3.0f, 1, -8}
            ))},
            engine::Scale{glm::vec3{0.5}},
            engine::Transparent{}
        );
    }

    // === === === === === === === ===
    // LIGHTING
    // === === === === === === === ===
    {
        reg.create( // sun
            engine::DynamicLight{},
            engine::DirectionalLight{
                .color = glm::vec3{1}
            },
            engine::Orientation{glm::angleAxis(
                0.0f,
                glm::normalize(glm::vec3{1, -1, 1})
            )}
        );
    }

    Controller::createCamera(reg, {0, 2, 0});
}