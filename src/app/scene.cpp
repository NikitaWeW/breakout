#include "scene.hpp"

#include "engine/DSA/Data.hpp"
#include "engine/DSA/ECS.hpp"
#include "engine/Engine.hpp"
#include "controller.hpp"

#include <fmt/ostream.h>
#include <random>

#include "engine/Resource/Loaders.hpp"
#include "engine/Resource/Resources.hpp"
#include "glm/gtx/quaternion.hpp"

struct ChangeAnimationsTag {};
struct SunTag {};
struct Bouncy {
    float height = 1;
    float currentOffset = 0;
    float speed = 0.1;
};


void updateScene(engine::Registry &reg, float deltatime)
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
    for(auto e : reg.view<SunTag, engine::Transform, engine::Version>())
    {
        glm::quat &orientation = reg.get<engine::Transform>(e).orientation;

        orientation = glm::rotate(orientation, deltatime * 0.1f, glm::normalize(glm::vec3{0, 0.5, 1}));
        reg.get<engine::Version>(e).increment();
    }

    for(auto e : reg.view<engine::Transform, Bouncy>())
    {
        auto &bouncy = reg.get<Bouncy>(e);
        auto &transform = reg.get<engine::Transform>(e);
        transform.position.y -= bouncy.currentOffset;
        if(bouncy.currentOffset < 0 || bouncy.currentOffset > bouncy.height)
            bouncy.speed = -bouncy.speed;
        bouncy.currentOffset += bouncy.speed * deltatime;
        transform.position.y += bouncy.currentOffset;
        
        if(reg.has<engine::Version>(e))
            reg.get<engine::Version>(e).increment();
    }
    
}