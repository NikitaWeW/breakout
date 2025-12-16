#include "scene.hpp"

#include "engine/Engine.hpp"
#include "controller.hpp"

#include <random>

#include "glm/gtx/quaternion.hpp"

static std::string printTexture(ecs::entity e_texture, engine::Registry const &reg)
{
    std::stringstream ss;
    auto const &texture = reg.get<engine::Texture>(e_texture);
    ss 
        << 'e' << e_texture << ", \"" 
        << texture.getPath() << "\", \t" 
        << texture.data.getDimensions() << ", \t" 
        << (texture.srgb ? "srgb" : "not srgb");
    return ss.str();
}
static void printModelData(ecs::entity e_model, engine::Registry const &registry)
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

void createScene(engine::Registry &reg)
{
    auto cube =    engine::ResourceManager::instance().loadFromFile(engine::ResourceType::MODEL, "res/models/cube.obj");
    auto suzanne = engine::ResourceManager::instance().loadFromFile(engine::ResourceType::MODEL, "res/models/suzanne.obj");
    auto arrow =   engine::ResourceManager::instance().loadFromFile(engine::ResourceType::MODEL, "res/models/arrow.glb");
    reg.get<engine::Material>(reg.get<engine::Model>(arrow).meshes[0].e_material).properties.albedo = {0.2, 0.3, 0.9, 1};
    
    // === === === === === === === ===
    // RANDOM STUFF
    // === === === === === === === ===
    { // braces so sections fold nicely in my editor
        reg.create( // some random cube
            engine::Instance{cube}, 
            engine::Transform{
                .position = {1, 1, -2},
                .orientation = glm::normalize(glm::angleAxis(1.0f, glm::vec3{-45, 90, 35}))
            }
        );
        reg.create( // some random monkey
            engine::Instance{suzanne},
            engine::Transform{
                .position = {-1, 1, 5},
                .orientation = glm::normalize(glm::angleAxis(-26.0f, glm::vec3{2, 2, -4})),
                .scale = glm::vec3{0.5}
            }
        );
        reg.create( // some random sphere
            engine::Instance{loader.load(engine::DataType::MODEL, "res/models/sphere.obj")}, 
            engine::Transform{
                .position = {-2, 1, 4},
                .scale = {0.5, 0.5, 0.5}
            }
        );
        reg.create(
            engine::Instance{loader.load(engine::DataType::MODEL, "res/models/lemon.glb")}, 
            engine::Transform{
                .position = {4, 0.5, -5},
                .orientation = glm::normalize(glm::angleAxis(46.0f, glm::vec3{2, 2, -4})),
            }
        );


        reg.create( // plane
            engine::Instance{cube}, 
            engine::Transform{
                .position = { 0, 0, -2 },
                .scale = { 100, 0, 100 }
            }
        );
        reg.create( // deccer cubes (working flawlessly)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/deccer_cubes/SM_Deccer_Cubes_Textured_Complex.glb")
            }, 
            engine::Transform{
                .position = {-5, 2, 0},
                .scale = glm::vec3{0.5}
            }
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
            engine::Transform{
                .position = {4, 0, -7},
                .scale = glm::vec3{0.01}
            },
            engine::CurrentAnimation{
                .name = "Survey"
            }
        ); 
        reg.create( // dancing buddy 0 (dances so hard he accelerates into the sky)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/Silly_Dancing.fbx")
            }, 
            engine::Transform{
                .position = {0, 0, -7},
                .scale = glm::vec3{0.01}
            },
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
            engine::Transform{
                .position = {-2, 0, -8},
                .scale = glm::vec3{0.01}
            },
            engine::CurrentAnimation{
                .name = "mixamo.com",
                .speed = 1
            }
        );
        reg.create( // animation test (made myself thus it looks so bad)
            engine::Instance{
                loader.load(engine::DataType::MODEL, "res/models/blob.glb")
            }, 
            engine::Transform{
                .position = {5, 0, -3},
                .scale = glm::vec3{0.5}
            },
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

        material.properties.albedo = {1, 0, 0, 0.5};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material) // copy transparent material and override the model material
            },
            engine::Transform{
                .position = {4, 1, 4},
                .scale = {0, 2, 2}
            },
            engine::Transparent{}
        );
        material.properties.albedo = {0, 1, 0, 0.5};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material)
            },
            engine::Transform{
                .position = {5.5, 1, 4},
                .scale = {0, 2, 2}
            },
            engine::Transparent{}
        );
        material.properties.albedo = {0, 0, 1, 0.5};
        reg.create(
            engine::Instance{
                .e_model = cube,
                .e_material = reg.create(material)
            },
            engine::Transform{
                .position = {7, 1, 4},
                .scale = {0, 2, 2}
            },
            engine::Transparent{}
        );
        material.properties.albedo = {0.9, 0.9, 1, 0.5};
        reg.create(
            engine::Instance{
                suzanne,
                .e_material = reg.create(material)
            },
            engine::Transform{
                .position = {2, 1, 4},
                .orientation = glm::normalize(glm::angleAxis(-54.3f, glm::vec3{3.0f, 1, -8})),
                .scale = glm::vec3{0.5}
            },
            engine::Transparent{}
        );
    }

    // === === === === === === === ===
    // LIGHTING
    // === === === === === === === ===
    {
        reg.create( // sun
            SunTag{},
            engine::DynamicLight{},
            engine::DirectionalLight{
                .color = glm::vec3{1, 0.9, 0.8} * 1.0f
            },
            engine::ShadowLight{
                .shadowMapSize = 2048,
                .farPlane = 500
            },
            engine::Transform{
                .orientation = glm::quatLookAt(glm::normalize(glm::vec3{0, -1, 0}), glm::vec3{1,0, 0})
            }
        );

        reg.create(
            engine::DynamicLight{},
            engine::SpotLight{
                .color = glm::vec3{0.1, 0.5, 0.9} * 10.0f
            },
            engine::ShadowLight{
                .shadowMapSize = 1024
            },
            engine::Transform{
                .position = {3, 2, 2},
                .orientation = glm::quatLookAt(
                    glm::normalize(glm::vec3{0, 0, 0} - glm::vec3{3, 2, 2}),
                    glm::vec3{1,0,0}
                )
            }
        );
        reg.create(
            engine::DynamicLight{},
            engine::SpotLight{
                .color = glm::vec3{0.9, 0.6, 0.3} * 10.0f
            },
            engine::ShadowLight{
                .shadowMapSize = 512
            },
            engine::Transform{
                .position = {3, 2, 6},
                .orientation = glm::quatLookAt(
                    glm::normalize(glm::vec3{4, 1, 4} - glm::vec3{3, 2, 6}),
                    glm::vec3{1,0,0}
                )
            }
        );

        reg.create(
            engine::DynamicLight{},
            engine::PointLight{
                .color = glm::vec3{0.9, 0.4, 0.9} * 10.0f
            },
            engine::ShadowLight{
                .shadowMapSize = 512
            },
            engine::Transform{
                .position = {-3, 2, -2}
            }
        );
        reg.create(
            engine::DynamicLight{},
            engine::PointLight{
                .color = glm::vec3{0.2, 0.9, 0.9} * 10.0f
            },
            engine::ShadowLight{
                .shadowMapSize = 1024
            },
            engine::Transform{
                .position = {2, 2, 1}
            }
        );

        reg.create<engine::Skybox>({
            loader.load(engine::DataType::CUBEMAP, "res/textures/citrus_orchard_road_puresky_2k.hdr")
        });
    }

    Controller::createCamera(reg, {0, 3, 0});
}

void updateScene(Registry &reg, float deltatime)
{
    // === === === === === === === ===
    // CYCLE ANIMATIONS
    // === === === === === === === ===
    for(auto e_instance : reg.view<ChangeAnimationsTag, engine::Instance>())
    {
        if(glm::mod<float>(glfwGetTime(), 5) < 0.01 && !reg.has<engine::AnimationTransition>(e_instance))
        {
            auto const &model = reg.get<engine::Model>(reg.get<engine::Instance>(e_instance).e_model);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, model.animations.size() - 1);
            
            std::string newAnimation = model.animations.at(distrib(gen)).name;
            auto const &newAnim = *std::find_if(model.animations.begin(), model.animations.end(), [&](engine::Animation const &animation){ return animation.name == newAnimation; });
            float duration = 0.5 * newAnim.durationTicks / newAnim.ticksPerSecond;

            // ENGINE_INFO("transition from {} to {} in {}s", current.name, newAnimation, duration);

            reg.emplace<engine::AnimationTransition>(e_instance, engine::AnimationTransition{
                .to = newAnimation,
                .factorPerSecond = 1 / duration,
                .easeFunction = engine::ease::inOutCubic,
            });
        }
    }

    // === === === === === === === ===
    // MOVE SUN
    // === === === === === === === ===
    for(auto e : reg.view<SunTag, engine::Transform>())
    {
        glm::quat &orientation = reg.get<engine::Transform>(e).orientation;

        orientation = glm::rotate(orientation, deltatime * 0.1f, glm::normalize(glm::vec3{0, 0, 1}));
    }
}