#pragma once
#include <functional>
#include "engine/DSA/SparseSet.hpp"

namespace engine
{
    // TODO: render graph
    struct RenderPass
    {
    public:
        using ID_t = unsigned; /// Id type alias.
    private:
        inline static ID_t mNextID = 1;
    public:
        ID_t id = 0; /// Unique id of the pass. 0 - invalid.
        std::vector<ID_t> depends; /// Ids of passes it depends on.
        std::function<void()> draw; /// Draw callback.

        /// @brief Set the unique id.
        inline void makeId() { id = mNextID++; }
    };

    class IRenderer
    {
    private:
        SparseSet<RenderPass> mPasses;
        RenderPass::ID_t mMainPass = 0;
    protected:
        bool validatePasses() const;
    public:
        IRenderer() = default;
        virtual ~IRenderer() = default;

        virtual void setup() = 0;
        virtual void draw() = 0;

        void addRenderPass(RenderPass const &pass);
        void setMainRenderPass(RenderPass const &pass);
        void removeRenderPass(RenderPass::ID_t id);
        inline SparseSet<RenderPass> const &getRenderPasses() const { return mPasses; }
        inline RenderPass::ID_t const &getMainRenderPass() const { return mMainPass; }
    };
} // namespace engine
