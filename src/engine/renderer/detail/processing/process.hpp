#pragma once
#include "detail/mesh.hpp"

namespace engine::renderer::detail
{
    detail::Mesh createMesh(engine::Model const &model);
    void processModels(ecs::registry &reg);
    void processTextures(ecs::registry &reg);
}
