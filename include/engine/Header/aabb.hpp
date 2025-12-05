#pragma once
#include "data.hpp"
#include "glm/glm.hpp"

namespace engine
{
    template<typename T = glm::vec3>
    struct AABB
    {
        T min{0};
        T max{0};

        AABB() = default;
        inline void growToInclude(T const &p)
        {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }
    };
} // namespace engine
