#include "ModelMatrixAssembler.hpp"
#include "engine/core/data.hpp"

void engine::ModelMatrixAssembler::update(ecs::registry &registry)
{
    for(auto e : registry.view_any_of<engine::Position, engine::Orientation, engine::OrientationEulerXYZ, engine::Scale>(ecs::exclude_t<engine::ModelMatrix, ModelMatrixAssemblerExclude>{}))
        registry.emplace<engine::ModelMatrix>(e, 1.0f);

    for(auto e : registry.view<engine::ModelMatrix>(ecs::exclude_t<ModelMatrixAssemblerExclude>{}))
    {
        glm::mat4 &mat = registry.get<engine::ModelMatrix>(e);
        mat = glm::mat4(1.0f);

        if(registry.has<engine::Position>(e))
            mat *= glm::translate(glm::mat4(1.0f), registry.get<engine::Position>(e));

        if(registry.has<engine::Orientation>(e))
            mat *= glm::mat4_cast(registry.get<engine::Orientation>(e));

        if(registry.has<engine::OrientationEulerXYZ>(e))
        {
            auto const &rot = registry.get<engine::OrientationEulerXYZ>(e);
            mat *= glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3{1, 0, 0}) 
                 * glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3{0, 1, 0}) 
                 * glm::rotate(glm::mat4(1.0f), rot.z, glm::vec3{0, 0, 1});
        }

        if(registry.has<engine::Scale>(e))
            mat *= glm::scale(glm::mat4(1.0f), registry.get<engine::Scale>(e));
    }
}