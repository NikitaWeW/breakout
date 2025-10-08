#pragma once
#include "engine/config.hpp"
#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "ecs.hpp"
#include "engine/ease_functions.hpp"

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
        engine::ease::FuncPtr easeFunction = engine::ease::linear;
    };

    class Animator
    {
    private:
    public:
        void update(ecs::registry &registry, float deltatime);
    };
} // namespace engine
