#include "engine/Renderer/Renderer.hpp"
#include "engine/DSA/SparseSet.hpp"
#include "engine/Header/Config.hpp"
#include "engine/Logging/Logging.hpp"

using namespace engine;

static bool validateGraph(SparseSet<RenderPass> const &passes, RenderPass const &pass, std::vector<RenderPass::ID_t> &ids)
{
    if(std::find(ids.begin(), ids.end(), pass.id) != ids.end())
    {
        ENGINE_ERROR("Dependency cycle in render pass graph detected! Id: {}.", pass.id);
    }

    for(auto id : pass.depends)
    {
        if(!passes.contains(id))
        {
            ENGINE_ERROR("Unknown dependent pass with id {}!", id);
            return false;
        }

        ids.push_back(id);
        if(!validateGraph(passes, passes.get(id), ids))
            return false;
        ids.pop_back();
    }

    return true;
}
bool IRenderer::validatePasses() const
{
    if(!mPasses.contains(mMainPass))
    {
        ENGINE_ERROR("Invalid main pass!");
        return false;
    }

    std::vector<RenderPass::ID_t> ids;
    return validateGraph(mPasses, mPasses.get(mMainPass), ids);
}


void IRenderer::addRenderPass(RenderPass const &pass)
{
    if(mPasses.contains(pass.id))
    {
        ENGINE_ERROR("Pass with id {} already exists in the renderer!");
        return;
    }
    
    mPasses.emplace(pass.id, pass);
}

void IRenderer::removeRenderPass(RenderPass::ID_t id)
{
    if(!mPasses.contains(id))
    {
        ENGINE_ERROR("Renderer doesent contain pass with id {}!", id);
        return;
    }

    mPasses.erase(id);
}

void IRenderer::setMainRenderPass(RenderPass const &pass)
{
    if(mMainPass)
    {
        ENGINE_ASSERT(mPasses.contains(mMainPass));
        mPasses.erase(mMainPass);
    }

    mMainPass = pass.id;
    addRenderPass(pass);
}
