#pragma once
#include "utils/ECS.hpp"
#include "utils/Model.hpp"

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
        float timeSeconds = 0;
        float speed = 1;
    };
    class Animator : public ecs::ISystem
    {
    public:
        void update(std::set<ecs::Entity_t> const &entities, double deltatime);
    };
} // namespace game
