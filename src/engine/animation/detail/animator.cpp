#include "engine/animation/animation.hpp"
#include "engine/data.hpp"
#include <algorithm>
#include <optional>

struct Keyframe
{
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 scale;
};

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
            current.time = glm::mod(current.time, 1.0f);
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
Keyframe interpolateKeyframes(Keyframe const &first, Keyframe const &second, float factor)
{
    return Keyframe{
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

glm::vec3 calculateInterpolatedPosition(engine::Animation::Keyframes const &keyframes, float time)
{
    auto const &keys = keyframes.positions;
    if(keys.empty())
        return {0, 0, 0};

    auto it = std::upper_bound(keys.begin(), keys.end(), time, [](float time, engine::Animation::PositionKey const &keyframe){ return keyframe.timeTicks > time; });

    if(it >= keys.end() || it == keys.begin())
        return keys.back().value;

    auto const &second = *it;
    auto const &first = *(it-1);

    float deltatime = second.timeTicks - first.timeTicks;
    float factor = (time - first.timeTicks) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);
    return glm::mix(
        first.value,
        second.value,
        factor
    );
}
glm::quat calculateInterpolatedOrientation(engine::Animation::Keyframes const &keyframes, float time)
{
    auto const &keys = keyframes.orientations;
    if(keys.empty())
        return {0, 0, 0, 1};

    auto it = std::upper_bound(keys.begin(), keys.end(), time, [](float time, engine::Animation::OrientationKey const &keyframe){ return keyframe.timeTicks > time; });

    if(it >= keys.end() || it == keys.begin())
        return keys.back().value;

    auto const &second = *it;
    auto const &first = *(it-1);

    float deltatime = second.timeTicks - first.timeTicks;
    float factor = (time - first.timeTicks) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);
    return glm::normalize(glm::slerp(
        first.value,
        second.value,
        factor
    ));
}
glm::vec3 calculateInterpolatedScale(engine::Animation::Keyframes const &keyframes, float time)
{
    auto const &keys = keyframes.scales;
    if(keys.empty())
        return {1, 1, 1};

    auto it = std::upper_bound(keys.begin(), keys.end(), time, [](float time, engine::Animation::ScaleKey const &keyframe){ return keyframe.timeTicks > time; });

    if(it >= keys.end() || it == keys.begin())
        return keys.back().value;

    auto const &second = *it;
    auto const &first = *(it-1);

    float deltatime = second.timeTicks - first.timeTicks;
    float factor = (time - first.timeTicks) / deltatime;
    factor = glm::clamp<float>(factor, 0, 1);
    return glm::mix(
        first.value,
        second.value,
        factor
    );
}

Keyframe calculateInterpolatedKeyframe(engine::Animation::Keyframes const &keyframes, float time)
{
    return Keyframe{
        .position    = calculateInterpolatedPosition(keyframes, time),
        .orientation = calculateInterpolatedOrientation(keyframes, time),
        .scale       = calculateInterpolatedScale(keyframes, time),
    };
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

        size_t numBones = animation.bones.size();
        current.boneMatrices.resize(numBones);
        current.localBoneMatrices.resize(numBones);

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

            for(size_t bone = 0; bone < numBones; ++bone)
            {
                auto first = calculateInterpolatedKeyframe(animation.bones.at(bone), current.time * animation.durationTicks);
                auto second = calculateInterpolatedKeyframe(secondAnimation.bones.at(bone), current.time * secondAnimation.durationTicks);
            
                auto result = interpolateKeyframes(first, second, transition.easeFunction(transition.factor));

                glm::mat4 transform = 
                    glm::translate(glm::mat4{1.0f}, result.position) * 
                    glm::mat4_cast(result.orientation) * 
                    glm::scale(glm::mat4{1.0f}, result.scale);

                current.localBoneMatrices.at(bone) = transform;
            }

            if(transition.factor >= 1) {
                current.name = transition.to;
                registry.remove<AnimationTransition>(entity);
            }
        }
        else
        {
            for(size_t bone = 0; bone < numBones; ++bone)
            {
                auto result = calculateInterpolatedKeyframe(animation.bones.at(bone), current.time * animation.durationTicks);

                glm::mat4 transform = 
                    glm::translate(glm::mat4{1.0f}, result.position) * 
                    glm::mat4_cast(result.orientation) * 
                    glm::scale(glm::mat4{1.0f}, result.scale);

                current.localBoneMatrices.at(bone) = transform;
            }
        }

        for(size_t bone = 0; bone < numBones; ++bone)
        {
            int parent = model.skeleton.parents.at(bone);
            auto &matrix = current.boneMatrices.at(bone);
            auto const &local = current.localBoneMatrices.at(bone);
            if(parent == -1)
                matrix = local;
            else
                matrix = current.boneMatrices.at(parent) * local;
        }
        for(size_t bone = 0; bone < numBones; ++bone)
        {
            auto &matrix = current.boneMatrices.at(bone);
            matrix = matrix * model.skeleton.bindTransform.at(bone);
        }
    }
}