#pragma once
#include "engine/DSA/ECS.hpp"

namespace engine
{
    /// @brief The renderer implementation interface.
    class IRenderer
    {
    public:
        IRenderer() = default;
        virtual ~IRenderer() = default;

        virtual void draw(Registry &) = 0;
    };
} // namespace engine
