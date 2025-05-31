#pragma once
#include "glm/glm.hpp"

template <typename vec_t = glm::vec3>
class AABB {
public:
    vec_t min = vec_t{0};
    vec_t max = vec_t{0};

    inline void growToInclude(vec_t const &point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
};