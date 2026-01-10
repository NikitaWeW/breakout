#pragma once

namespace engine
{
    /// @brief The renderer implementation interface.
    class IRenderer
    {
    public:
        IRenderer() = default;
        virtual ~IRenderer() = default;

        virtual void draw() = 0;
    };
} // namespace engine
