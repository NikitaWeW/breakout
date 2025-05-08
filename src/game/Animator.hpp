#pragma once
#include "utils/ECS.hpp"
#include "utils/Model.hpp"
#include "EaseFunctions.hpp"

namespace game
{
    enum AnimationRepeatMode
    {
        EXIT, LOOP, MIRROR
    };
    struct Animation
    { 
        AnimationRepeatMode repeatMode = LOOP;
        aiAnimation const *aianimation = nullptr;
        std::vector<glm::mat4> const *boneMatrices = nullptr;
        float normalizedTime = 0; // time ranging from 0 to 1 (time / duration)
        float speed = 1;
    };
    struct AnimationTransition
    {
        Animation to;
        float factor = 0;
        float factorPerSecond = 1; // how much does transition factor change per second
        easeFunc::easeFuncPtr easeFunction = easeFunc::liniar;
    };
    class Animator : public ecs::ISystem
    {
    public:
        void update(std::set<ecs::Entity_t> const &entities, double deltatime);
    };
} // namespace game
