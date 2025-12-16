#include "engine/Resource/ResourceManager.hpp"
#include "engine/Header/Config.hpp"
#include "engine/Header/Exceptions.hpp"
#include "engine/DSA/SparseSet.hpp"
#include "engine/Logging/Logging.hpp"
#include <unordered_map>

using namespace engine;
constexpr static ResourceID_t INVALID_RESOURCE_ID = 0;

struct ResourceManagerImpl
{
    std::unordered_map<ResourceType, std::unique_ptr<ILoader>> mLoaders;
    struct ResourceData
    {
        unsigned numReferences = 0;
        ResourceType type = ResourceType::INVALID;
        ResourceID_t id = 0;
        std::unique_ptr<IResource> data;
    };
    SparseSet<ResourceData> mResourceData;

    // FIXME: Might eat some space with a big amount of resources because its used as a per type sparse set index and is not per type.
    ResourceID_t mNextID = 1;
};

ResourceManager &ResourceManager::instance() 
{
    static auto mgr = std::make_shared<ResourceManager>();
    return *mgr;
}

unsigned &ResourceManager::refCount(ResourceID_t id)
{
    return unwrap()->mResourceData[id].numReferences;
}
ResourceHandle ResourceManager::loadFromFile(ResourceType type, std::string_view path, LoaderOptions_t options)
{
    auto impl = unwrap();
    if(impl->mLoaders.find(type) == impl->mLoaders.end())
    {

        throw InvalidResourceLoaderError{"ResourceManager::loadFromFile: unknown resource type!"};
    }

    auto pResource = std::find_if(impl->mResourceData.data().begin(), impl->mResourceData.data().end(), [path](auto const &x){
        return x.data->getPath() = path
    });
    bool loaded = pResource != impl->mResourceData.data().end() && pResource->data.get() != nullptr;
    auto id = loaded ? pResource->id : impl->mNextID++;
    if(!loaded)
    {
        auto &loader = impl->mLoaders.at(type);
        auto res = loader->loadFromFile(path, options);
        if(!res)
        {
            ENGINE_CORE_ERROR("Failed to load resource \"{}\"", path);
            return ResourceHandle{};
        }
        if(res->getPath() == "")
            res->getPath() = path;
        ENGINE_ASSERT(!impl->mResourceData.contains(id));
        impl->mResourceData[id] = ResourceManagerImpl::ResourceData{
            .id = id,
            .data = std::move(res)
        };
    }

    return ResourceHandle(weak_from_this(), id);
}
ResourceHandle ResourceManager::loadFromMemory(ResourceType type, void const *data, size_t size, LoaderOptions_t options)
{
    auto impl = unwrap();
    if(impl->mLoaders.find(type) == impl->mLoaders.end())
    {
        throw InvalidResourceLoaderError{"ResourceManager::loadFromMemory: unknown resource type!"};
    }

    auto id = impl->mNextID++;
    auto &loader = impl->mLoaders.at(type);
    auto res = loader->loadFromMemory(data, size, options);
    if(!res)
    {
        ENGINE_CORE_ERROR("Failed to load resource from memory!");
        return ResourceHandle{};
    }
    if(res->getPath() == "")
        res->getPath() = "from memory " + std::to_string(UID{}.value());
    ENGINE_ASSERT(!impl->mResourceData.contains(id));
    impl->mResourceData[id] = ResourceManagerImpl::ResourceData{
        .id = id,
        .data = std::move(res)
    };

    return ResourceHandle(weak_from_this(), id);
}
ResourceHandle ResourceManager::addRawResource(ResourceType type, std::unique_ptr<IResource> &&res)
{
    auto impl = unwrap();
    auto id = impl->mNextID++;

    if(!res)
    {
        ENGINE_CORE_ERROR("Failed to add raw resource (nullptr)!");
        return ResourceHandle{};
    }
    if(res->getPath() == "")
        res->getPath() = "raw resource " + std::to_string(UID{}.value());

    ENGINE_ASSERT(!impl->mResourceData.contains(id));
    impl->mResourceData[id] = ResourceManagerImpl::ResourceData{
        .id = id,
        .data = std::move(res)
    };

    return ResourceHandle(weak_from_this(), id);
}
ResourceRes_t ResourceManager::getResource(ResourceHandle const &handle) const
{
    if(!handle)
        return std::nullopt;

    auto &storage = unwrap()->mResourceData;
    ENGINE_ASSERT(storage.contains(handle.getID()));

    if(!storage.contains(handle.getID()))
        return std::nullopt;

    return *storage.get(handle.getID()).data;
}
void ResourceManager::registerLoader(ResourceType type, std::unique_ptr<ILoader> &&loader)
{
    unwrap()->mLoaders[type] = std::move(loader);
}

void ResourceManager::doGarbageCollection()
{
    auto impl = unwrap();
    static std::vector<ResourceID_t> toDelete;
    toDelete.clear();
    toDelete.reserve(impl->mResourceData.data().size() / 4);
    for(auto [id, data] : impl->mResourceData)
    {
        if(data.numReferences == 0)
            toDelete.emplace_back(id);
    }
    for(auto const &id : toDelete)
        impl->mResourceData.erase(id);
}
ResourceHandle::ResourceHandle(std::weak_ptr<ResourceManager> &&manager, ResourceID_t id) : mManager(std::move(manager)), mID(id)
{
    if(valid())
        ++mManager.lock()->refCount(mID);
}
ResourceHandle::ResourceHandle(ResourceHandle const &other)
{
    *this = other;
}
ResourceHandle::ResourceHandle(ResourceHandle &&other)
{
    *this = std::move(other);
}
ResourceHandle::~ResourceHandle() 
{
    if(valid())
        --mManager.lock()->refCount(mID);
}
ResourceHandle &ResourceHandle::operator=(ResourceHandle const &other) noexcept 
{
    mManager = other.mManager;
    mID = other.mID;
    if(valid())
        ++mManager.lock()->refCount(mID);
    return *this;
}
ResourceHandle &ResourceHandle::operator=(ResourceHandle &&other) noexcept = default;
ResourceRes_t ResourceHandle::getResource() const
{
    if(!valid())
        throw InvalidResourceHandleError{"ResourceHandle::getResource from an invalid handle!"};
    return mManager.lock()->getResource(*this);
}
bool ResourceHandle::valid() const
{
    return !mManager.expired() && getID() != 0;
}
static bool sameOwner(std::weak_ptr<ResourceManager> const &a,std::weak_ptr<ResourceManager> const &b)
{
    return !a.owner_before(b) && !b.owner_before(a);
}
bool ResourceHandle::operator==(ResourceHandle const &other) const noexcept
{
    return mID == other.mID && sameOwner(mManager, other.mManager);
}
bool ResourceHandle::operator!=(ResourceHandle const &other) const noexcept
{
    return !(*this == other);
}
bool ResourceHandle::operator<(ResourceHandle const &other) const noexcept
{
    if(!sameOwner(mManager, other.mManager))
        return mManager.owner_before(other.mManager);

    return mID < other.mID;
}
