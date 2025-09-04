#pragma once
#include "engine/renderer/renderer.hpp"
#include "processing/process.hpp"
#include "engine/config.hpp"

void engine::renderer::processData(ecs::registry &reg)
{
    ENGINE_PROFILE();
    detail::processModels(reg);
    detail::processTextures(reg);
}
