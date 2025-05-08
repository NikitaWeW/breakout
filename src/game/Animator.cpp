#include "Animator.hpp"

float getDurationSeconds(game::Animation const &animation)
{
    float ticksPerSecond = (float) (animation.aianimation->mTicksPerSecond != 0 ? animation.aianimation->mTicksPerSecond : 25.0f);
    float animationDurationSeconds = animation.aianimation->mDuration / ticksPerSecond;
    animationDurationSeconds = glm::max(animationDurationSeconds, 0.0001f);
    return animationDurationSeconds;
}

void updateAnimation(game::Animation &animation, float deltatime)
{
    if(!animation.aianimation) return;
    animation.normalizedTime += (deltatime * animation.speed) / getDurationSeconds(animation);

    if(animation.normalizedTime <= 0 || animation.normalizedTime >= 1) {
        switch (animation.repeatMode)
        {
        case game::AnimationRepeatMode::LOOP:
            animation.normalizedTime -= glm::floor(animation.normalizedTime);
            break;
        case game::AnimationRepeatMode::EXIT:
            animation.aianimation = nullptr;
            break;
        case game::AnimationRepeatMode::MIRROR:
            animation.speed = -animation.speed;
            break;
            
        default:
            animation.normalizedTime = 0;
            break;
        }
    }
    animation.normalizedTime = glm::clamp<float>(animation.normalizedTime, 0, 1);
}

void game::Animator::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &entity : entities)
    {
        if(!ecs::entityHasComponent<Animation>(entity)) continue;
        Animation &animation = ecs::get<Animation>(entity);

        updateAnimation(animation, deltatime);
        if(!animation.aianimation) continue;

        if(ecs::entityHasComponent<AnimationTransition>(entity)) {
            AnimationTransition &transition = ecs::get<AnimationTransition>(entity);
            transition.to.normalizedTime = animation.normalizedTime;

            if(transition.to.aianimation) {
                transition.factor += transition.factorPerSecond * deltatime;
    
                if(ecs::entityHasComponent<model::Model>(entity)) {
                    animation.boneMatrices = &ecs::get<model::Model>(entity).getBoneTransformations(
                        animation.normalizedTime * getDurationSeconds(animation), transition.to.normalizedTime * getDurationSeconds(transition.to), 
                        animation.aianimation, transition.to.aianimation, 
                        transition.easeFunction(transition.factor)
                    );
                }
    
                if(transition.factor >= 1) {
                    ecs::removeComponent<AnimationTransition>(entity);
                    animation = transition.to;
                }
            } else {
                ecs::removeComponent<AnimationTransition>(entity);
            }
        }
        if(ecs::entityHasComponent<model::Model>(entity) && !ecs::entityHasComponent<AnimationTransition>(entity)) {
            animation.boneMatrices = &ecs::get<model::Model>(entity).getBoneTransformations(animation.normalizedTime * getDurationSeconds(animation), animation.aianimation);
        }
    }
}