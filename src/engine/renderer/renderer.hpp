#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "engine/data.hpp"

// implementation
#include "detail/renderer.hpp"

/**
 * \brief The renderer abstraction. 
 * The main motivation behind this kind of structure is that one can switch the renderer entirely just by switching the detail dir.
 */
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
        /** \brief The reference to the implementation specific data created from the processed data. */
        ecs::entity data;
    };

    struct ProcessedModel : public Processed {};
    struct ProcessedTexture : public Processed {};

    /** \brief A command to draw an object. */
    struct Draw
    {
        /** \brief A reference to the Processed engine::Model. */
        ecs::entity model;
    };

    /** \brief The renderer settings. */
    struct RendererContext
    {
        Camera camera;
        size_t shadowMapSize = 1024;
        float shadowMapNearPlane = 0.01f;
        float shadowMapFarPlane = 100;
    };

    /** \brief Sets up the renderer, pipeline, processes meshes. */
    inline constexpr void (*setup)(ecs::registry&) = detail::setup;

    /** \brief Draw all the Draw components in the registry (if valid). */
    inline constexpr void (*render)(ecs::registry&) = detail::render;

    /**
     * \brief Processes external data such as models and textures and creates the data used by renderer.
     * As an example, the mesh data will be translated into the graphics api buffers.
     * Adds a Processed component.
     */
    inline constexpr void (*processData)(ecs::registry&) = detail::processData;
} // namespace engine::renderer
