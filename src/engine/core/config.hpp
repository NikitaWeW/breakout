#pragma once

#include <cassert>

#define ENGINE_ASSERT(x) assert(x)
#define ENGINE_ASSERT_MSG(x, m) ENGINE_ASSERT((x) && m)
#define ENGINE_PROFILE()

#ifndef ECS_ASSERT
#define ECS_ASSERT ENGINE_ASSERT_MSG
#define ECS_PROFILE ENGINE_PROFILE
#else
#warning ecs.hpp included before the engine/confing.hpp, the engine configuration wont apply to ecs.
#endif

// #define ENGINE_NO_OUTPUT
