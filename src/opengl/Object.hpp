#pragma once
#include "utils/Resource.hpp"

namespace opengl 
{
    class Object : public Resource 
    {
    protected:
        unsigned m_renderID = 0;
    public:
        inline unsigned getRenderID() noexcept { return m_renderID; }
        virtual void bind(unsigned slot = 0) const noexcept = 0;
    };
}; // namespace opengl