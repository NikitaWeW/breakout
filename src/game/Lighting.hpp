#pragma once
#include "glm/glm.hpp"

namespace game
{
    struct Light {
        float intencity;
        float color;
        float attenuation;
    };
    struct PointLight {};
    struct DirectionalLight {};
    struct SpotLight {
        float innerConeAngle;
        float outerConeAngle;
    };
    // TODO: implement
    struct AreaLight {
        glm::vec2 scale;
    };
} // namespace game
