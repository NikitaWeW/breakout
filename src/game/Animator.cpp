#include "Animator.hpp"

std::ostream &operator<<(std::ostream &os, glm::mat4 const &mat)
{
    os << std::setfill(' ') << std::setprecision(3);
    for(unsigned row = 0; row < 4; ++row) {
        for(unsigned col = 0; col < 4; ++col) {
            os << std::setw(10) << mat[col][row];
        }
        os << '\n';
    }

    return os;
}

void game::Animator::update(std::set<ecs::Entity_t> const &entities, double deltatime)
{
    for(ecs::Entity_t const &entity : entities)
    {
        if(!ecs::entityHasComponent<Animation>(entity)) continue;
        Animation &animation = ecs::get<Animation>(entity);
        if(animation.aianimation == nullptr) continue;
        animation.timeSeconds += deltatime * animation.speed;
        float ticksPerSecond = (float) (animation.aianimation->mTicksPerSecond != 0 ? animation.aianimation->mTicksPerSecond : 25.0f);
        float animationDurationSeconds = animation.aianimation->mDuration / ticksPerSecond;
        if(animation.timeSeconds >= animationDurationSeconds || animation.timeSeconds <= 0) {
            switch (animation.repeatMode)
            {
            case AnimationRepeatMode::LOOP:
                animation.timeSeconds = 0;
                break;
            case AnimationRepeatMode::EXIT:
                ecs::removeComponent<Animation>(entity);
                break;
            case AnimationRepeatMode::MIRROR:
                animation.speed = -animation.speed;
                animation.timeSeconds = glm::clamp<float>(animation.timeSeconds, 0, animationDurationSeconds);
                break;
                
            default:
                animation.timeSeconds = 0;
                break;
            }
        }

        if(ecs::entityHasComponent<model::Model>(entity)) {
            animation.boneMatrices = &ecs::get<model::Model>(entity).getBoneTransformations(animation.timeSeconds, animation.aianimation);
        }
    }
}