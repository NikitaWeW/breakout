#pragma once

#include <cassert>

#ifndef ENGINE_ASSERT
#define ENGINE_ASSERT(x) assert(x)
#endif
#define ENGINE_ASSERT_MSG(x, m) ENGINE_ASSERT((x) && m)

#ifndef ECS_ASSERT
#define ECS_ASSERT ENGINE_ASSERT_MSG
#else
#warning ecs.hpp included before the engine/Header/Confing.hpp, the engine configuration will not apply to ecs.
#endif

// Disable logging
// #define ENGINE_NO_LOG

