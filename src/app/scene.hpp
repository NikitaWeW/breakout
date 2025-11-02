#pragma once
#include "engine/engine.hpp"

struct ChangeAnimationsTag {};

inline void createScene(ecs::registry &reg)
{
    engine::Loader loader{reg};
    auto cube =    loader.load(engine::DataType::MODEL, "res/models/cube.obj");
    auto suzanne = loader.load(engine::DataType::MODEL, "res/models/suzanne.obj");

    reg.create(
        engine::Instance{cube}, 
        engine::Position{{1, 1, -2}},
        engine::OrientationEulerXYZ{{-45, 90, 35}}
    );
    reg.create(
        engine::Instance{suzanne},
        engine::Position{{-1, 1, 5}},
        engine::Orientation{glm::angleAxis(
            -26.0f,
            glm::vec3{1.0f, 2, -4}
        )},
        engine::Scale{glm::vec3{0.1}}
    );
    reg.create(
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
    reg.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/Silly_Dancing.fbx")
        }, 
        engine::Position{{0, 0, -7}},
        engine::Scale{glm::vec3{0.01}},
        engine::Acceleration{.values = {{engine::UID{}, {0, 0.01, 0}}}},
        engine::Velocity{},
        engine::CurrentAnimation{
            .name = "mixamo.com",
            .speed = 4
        }
    );
    reg.create(
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
    reg.create(
        engine::Instance{
            loader.load(engine::DataType::MODEL, "res/models/deccer_cubes/SM_Deccer_Cubes_Textured_Complex.glb")
        }, 
        engine::Position{{-5, 0, 0}},
        engine::Scale{glm::vec3{0.5}}
    );
    reg.create(
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


    reg.create(
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