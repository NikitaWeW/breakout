#pragma once
#include "engine/DSA/ECS.hpp"

struct ChangeAnimationsTag {};
struct SunTag {};

void createScene(engine::Registry &reg);
void updateScene(engine::Registry &reg, float deltatime);
