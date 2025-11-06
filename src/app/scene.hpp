#pragma once
#include "engine/core/ecs.hpp"

struct ChangeAnimationsTag {};
struct SunTag {};

void createScene(ecs::registry &reg);
void updateScene(ecs::registry &reg, float deltatime);
