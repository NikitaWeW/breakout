#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace game
{
    struct Position              : glm::vec3 {};
    struct OrientationEuler      : glm::vec3 {};
    struct Scale                 : glm::vec3 {};
    struct Velocity              : glm::vec3 {};
    struct OrientationQuaternion : glm::quat {};
    struct Direction             : glm::vec3 {};
} // namespace game
