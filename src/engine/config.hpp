#pragma once

#include <cassert>
#include <iostream>

#ifndef ENGINE_NO_OUTPUT
#define ENGINE_NO_OUTPUT false
#endif

#define ENGINE_ASSERT(x, msg) (assert((x) && msg))
#define ENGINE_PROFILE()
#define ENGINE_OUT if(ENGINE_NO_OUTPUT) {} else std::cout

#ifndef ECS_ASSERT
#define ECS_ASSERT ENGINE_ASSERT
#define ECS_PROFILE ENGINE_PROFILE
#else
#warning ecs.hpp included before the engine/confing.hpp, the engine configuration wont apply to ecs.
#endif
