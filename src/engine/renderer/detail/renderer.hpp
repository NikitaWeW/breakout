#pragma once

namespace engine::renderer::detail
{
    /** \brief Sets up the renderer, pipeline, processes meshes. */
    void setup(ecs::registry &reg);
    /** \brief Draw all the Draw components in the registry (if valid). */
    void render(ecs::registry &reg);

    /**
     * \brief Processes external data such as models and textures and creates the data used by renderer.
     * As an example, the mesh data will be translated into the graphics api buffers.
     * Adds a Processed component.
     * Is called at setup.
     */
    void processData(ecs::registry &reg);
} // namespace engine::renderer::detail
