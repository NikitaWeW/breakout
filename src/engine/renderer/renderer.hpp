#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "engine/data.hpp"

namespace engine
{
    /** \brief A command to draw an object. */
    struct Draw
    {
        /** \brief A reference to the Processed engine::Model. */
        ecs::entity model;
    };

    /**
     * \brief The renderer implementation interface.
     */
    class IRenderer
    {
    public:
        IRenderer() = default;
        virtual ~IRenderer() = default;

        /** \brief Sets up the renderer, pipeline, etc. */
        virtual inline void setup(ecs::registry &reg) {};

        /**
         * \brief Processes external data such as models and textures and creates the data used by renderer.
         * As an example, the mesh data will be translated into the graphics api buffers.
         */
        virtual inline void processData(ecs::registry &reg) {};
        
        /** \brief Draw all the Draw components in the registry (if valid). */
        virtual void draw(ecs::registry &reg) = 0;
    };
} // namespace engine
