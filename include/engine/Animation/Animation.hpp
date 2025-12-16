#pragma once
#include <string>
#include <vector>
#include "engine/DSA/ECS.hpp"
#include "engine/DSA/EaseFunctions.hpp"
#include "engine/DSA/Data.hpp"

namespace engine
{
    struct CurrentAnimation
    {
        std::string name;
        enum class RepeatMode
        {
            EXIT, LOOP, MIRROR
        } repeat = RepeatMode::LOOP;
        float time = 0; // normalized
        float speed = 1;
        std::vector<glm::mat4> boneMatrices;
        // used to avoid unnecessary allocations to construct hierarchy-free boneMatrices, but can be used too.
        std::vector<glm::mat4> localBoneMatrices;
    };
    struct AnimationTransition
    {
        std::string to;
        float factor = 0;
        float factorPerSecond = 1; // how much does transition factor change per second
        ease::FuncPtr easeFunction = ease::linear;
    };

    class Animator
    {
    protected:
        void animate(Registry &registry, ecs::entity entity, float deltatime);

    public:
        void calculateBoneMatrices(std::vector<glm::mat4> &boneMatrices, Model::Skeleton const &skeleton, float normalizedTime, Animation const &animation, Animation const *secondAnimation = nullptr, AnimationTransition transition = {});
        void update(Registry &registry, float deltatime);
    };

    // /// @brief Creates retargeted animations in the destModel and changes its skeleton.
    // void retargetSkeleton(Model &destModel, Model const &srcModel);


    Animation const *findAnimation(Model const &model, std::string_view name);
} // namespace engine
