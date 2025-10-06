#include "engine/animation/animation.hpp"
#include "engine/data.hpp"
#include <algorithm>
#include <optional>

float getDurationSeconds(engine::Animation const &animation)
{
    float ticksPerSecond = animation.ticksPerSecond;
    ENGINE_ASSERT(ticksPerSecond != 0);
    float animationDurationSeconds = animation.durationTicks / ticksPerSecond;
    animationDurationSeconds = glm::max(animationDurationSeconds, 0.0001f);
    return animationDurationSeconds;
}

bool updateAnimation(engine::Model const &model, engine::CurrentAnimation &current, engine::Animation const &animation, float deltatime)
{
    current.time += (deltatime * current.speed) / getDurationSeconds(animation);

    bool exited = false;

    if(current.time <= 0 || current.time >= 1) {
        switch (current.repeat)
        {
        case engine::CurrentAnimation::RepeatMode::LOOP:
            current.time -= glm::floor(current.time);
            break;
        case engine::CurrentAnimation::RepeatMode::EXIT:
            exited = true;
            break;
        case engine::CurrentAnimation::RepeatMode::MIRROR:
            current.speed = -current.speed;
            break;
            
        default:
            current.time = 0;
            break;
        }
    }
    current.time = glm::clamp<float>(current.time, 0, 1);

    return exited;
}
engine::Animation::Keyframe interpolateKeyframes(engine::Animation::Keyframe const &first, engine::Animation::Keyframe const &second, float factor)
{
    return engine::Animation::Keyframe{
        .position = glm::mix(
            first.position, 
            second.position,
            factor
        ),
        .orientation = glm::normalize(glm::slerp(
            first.orientation,
            second.orientation,
            factor
        )),
        .scale = glm::mix(
            first.scale, 
            second.scale,
            factor
        )
    };
}
engine::Animation::Keyframe calculateInterpolatedKeyframe(std::vector<engine::Animation::Keyframe> const &keyframes, float time)
{
    ENGINE_ASSERT(keyframes.size() > 0);

    if(keyframes.size() == 1)
        return keyframes.back();

    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time, [](engine::Animation::Keyframe const &first, float time){ return first.timeTicks < time; });

    if(it+1 >= keyframes.end())
        return keyframes.back();

    auto const &first = *it;
    auto const &second = *(it+1);

    float deltatime = second.timeTicks - first.timeTicks;
    float factor = (time - first.timeTicks) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);

    return interpolateKeyframes(first, second, factor);
}

void engine::Animator::update(ecs::registry &registry, float deltatime)
{
    for(ecs::entity entity : registry.view<engine::Instance, engine::CurrentAnimation>())
    {
        auto const &model = registry.get<engine::Model>(registry.get<engine::Instance>(entity).e_model);
        auto &current = registry.get<engine::CurrentAnimation>(entity);
        auto animationIt = std::find_if(model.animations.begin(), model.animations.end(), [&](engine::Animation const &animation){ return animation.name == current.name; });
        if(animationIt == model.animations.end())
        {
            ENGINE_CORE_ERROR("Invalid animation \"{}\" in model \"{}\"", current.name, model.path);
            ENGINE_ASSERT_MSG(false, "Invalid animation");
            continue;
        }
        auto const &animation = *animationIt;

        if(updateAnimation(model, current, animation, deltatime))
        {
            registry.remove<engine::CurrentAnimation>(entity);
            continue;
        }

        float time = current.time * animation.durationTicks;

        size_t numBones = animation.bones.size();
        current.boneMatrices.resize(numBones);

        if(registry.has<AnimationTransition>(entity)) {
            AnimationTransition &transition = registry.get<AnimationTransition>(entity);
            auto secondAnimationIt = std::find_if(model.animations.begin(), model.animations.end(), [&](engine::Animation const &animation){ return animation.name == transition.to; });
            if(secondAnimationIt == model.animations.end())
            {
                ENGINE_CORE_ERROR("Invalid animation transition \"{}\" in model \"{}\"", transition.to, model.path);
                ENGINE_ASSERT_MSG(false, "Invalid animation");
                return;
            }
            auto const &secondAnimation = *secondAnimationIt;

            transition.factor += transition.factorPerSecond * deltatime;

            for(size_t i = 0; i < numBones; ++i)
            {
                auto &matrix = current.boneMatrices.at(i);

                auto first = calculateInterpolatedKeyframe(animation.bones.at(i), time);
                auto second = calculateInterpolatedKeyframe(secondAnimation.bones.at(i), time);
            
                auto result = interpolateKeyframes(first, second, transition.easeFunction(transition.factor));

                matrix = 
                    glm::translate(glm::mat4{1.0f}, result.position) * 
                    glm::mat4_cast(result.orientation) * 
                    glm::scale(glm::mat4{1.0f}, result.scale);
            }

            if(transition.factor >= 1) {
                current.name = transition.to;
                registry.remove<AnimationTransition>(entity);
            }
        }
        else
        {
            for(size_t i = 0; i < numBones; ++i)
            {
                auto &matrix = current.boneMatrices.at(i);
                auto result = calculateInterpolatedKeyframe(animation.bones.at(i), time);

                matrix = 
                    glm::translate(glm::mat4{1.0f}, result.position) * 
                    glm::mat4_cast(result.orientation) * 
                    glm::scale(glm::mat4{1.0f}, result.scale);
            }
        }
    }
}