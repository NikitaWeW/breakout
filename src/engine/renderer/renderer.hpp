#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "glad/gl.h"
#include "GLFW/glfw3.h"

namespace engine::renderer
{
    /**
     * \brief The window representation. 
     * The registry needs to hold at least one of these at the setup.
     */
    struct Window
    {
        GLFWwindow *glfwWindow;
    };

    /** \brief A tag indicating the model is processed by renderer. */
    struct Processed 
    {
        /** \brief The pointer to the implementation specific data created from the processed data. */
        ecs::entity data;
    };

    /** \brief A command to draw an object. */
    struct Draw
    {
        /** \brief A pointer to the Processed model. */
        ecs::entity model;
    };

    /** \brief Sets up the renderer, pipeline, processes meshes. */
    void setup(ecs::registry &reg);

    /** \brief Draw all the Draw components in the registry (if valid). */
    void render(ecs::registry &reg);

    /**
     * \brief Processes external data such as models and textures and creates the data used by renderer.
     * As an example, the mesh data will be translated into the graphics api buffers.
     * Adds a Processed component.
     */
    void processData(ecs::registry &reg);
} // namespace engine::renderer
