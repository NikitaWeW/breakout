#pragma once
#include "engine/core/ecs.hpp"
#include "engine/core/data.hpp"

namespace engine
{
    /// \brief The renderer implementation interface.
    class IRenderer
    {
    public:
        IRenderer() = default;
        virtual ~IRenderer() = default;

        /// \brief Sets up the renderer, pipeline, etc.
        virtual inline void setup(ecs::registry &) {};

        /// \brief Processes external data such as models and textures and creates the data used by renderer.
        /// As an example, the mesh data will be translated into the graphics api buffers.
        virtual inline void processData(ecs::registry &) {};
        
        /// \brief Draw all the Draw components in the registry (if valid).
        virtual void draw(ecs::registry &) = 0;
    };
} // namespace engine
