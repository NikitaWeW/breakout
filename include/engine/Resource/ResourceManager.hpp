#pragma once
#include "engine/Header/Handle.hpp"
#include "engine/Header/UID.hpp"
#include "engine/Header/Config.hpp"
#include <string>
#include <memory>
#include <optional>

namespace engine
{

/// @brief Guess what.
enum class ResourceType
{
    INVALID = 0,
    MODEL,
    BITMAP,
    CUBEMAP,
    AUDIO,
    BUFFER
};

/// @brief An interface indicating the class is loadable by a resource manager. Some kind of resource data.
class IResource
{
private:
    std::string mPath;
public:
    IResource() = default;
    virtual ~IResource() = default;

    /// @brief Get the path to the resource. Every resource must have a unique path.
    /// @return A view on a path.
    inline std::string_view getPath() const { return mPath; }
    /// @brief Get the path to the resource. Every resource must have a unique path.
    /// @return A reference to a path.
    inline std::string &getPath() { return mPath; }
};

/// @brief An interface for the loader options.
/// IMPORTANT: This is an aggregate class with no virtual destructor. Delete the actual derived class, not the base class.
/// FIXME: maybe use std::any for options?
class ILoaderOptions {};

/// @brief An alias for options passed to the ILoader.
using LoaderOptions_t = std::optional<std::reference_wrapper<ILoaderOptions>>;

/// @brief An interface for the resource loaders.
class ILoader
{
public:
    ILoader() = default;
    virtual ~ILoader() = default;

    /// @brief Load a resource from a file.
    /// @param path The file path.
    /// @param options Optional optins that **must** be corresponding to the loader (e.g. ModelLoaderOptions for ModelLoader).
    /// @return A unique pointer pointing to the loaded resource. nullptr if failed to load.
    virtual std::unique_ptr<IResource> loadFromFile(std::string_view path, LoaderOptions_t options = std::nullopt) = 0;

    /// @brief Load a resource from memory.
    /// @param data A pointer to the resource data to load.
    /// @param size The size of the @p data.
    /// @param options Optional optins that **must** be corresponding to the loader (e.g. ModelLoaderOptions for ModelLoader).
    /// @return A unique pointer pointing to the loaded resource. nullptr if failed to load.
    virtual std::unique_ptr<IResource> loadFromMemory(void const *data, size_t size, LoaderOptions_t options = std::nullopt) = 0;
};

/// @brief An optional reference to a resource.
using ResourceRes_t = std::optional<std::reference_wrapper<IResource>>;
using ResourceID_t = unsigned;

class ResourceManager;

/// @brief A lightweight handle of a resource.
/// This is the intendent way to store resources in an application.
class ResourceHandle
{
private:
    std::weak_ptr<ResourceManager> mManager;
    ResourceID_t mID{};
public:
    /// @brief Create an invalid handle.
    ResourceHandle() = default;
    /// @brief Create a valid handle.
    ResourceHandle(std::weak_ptr<ResourceManager> &&manager, ResourceID_t id);
    ResourceHandle(ResourceHandle const &other);
    ResourceHandle(ResourceHandle &&other);
    ~ResourceHandle();
    
    ResourceHandle &operator=(ResourceHandle const &other);
    ResourceHandle &operator=(ResourceHandle &&other) noexcept;
    
    bool operator==(ResourceHandle const &other) const;
    bool operator!=(ResourceHandle const &other) const noexcept;
    /// @brief Weak ordering
    bool operator<(ResourceHandle const &other) const noexcept;
    
    /// @brief Get the resource the handle is pointing to.
    /// @throws InvalidResourceHandleError If the handle is invalid.
    ResourceRes_t getResource() const;
    /// @brief Check if the handle points to a valid resource.
    bool valid() const;

    /// @brief Get the id of the resource. Id 0 is invalid.
    inline ResourceID_t getID() const { return mID; }
    /// @copydoc valid
    inline operator bool() const { return valid(); }
};

class ResourceManager : public Handle<struct ResourceManagerImpl>, public std::enable_shared_from_this<ResourceManager>
{
private:
    ResourceManager() = default;
    friend ResourceHandle; // For reference counting
    unsigned &refCount(ResourceID_t id);
public:
    /// @brief Singleton getter
    static ResourceManager &instance();

    ResourceManager(ResourceManager const &) = delete;
    ResourceManager &operator=(ResourceManager const &) = delete;

    /// @brief Load a resource of a specific type from a file.
    /// Multiple calls for the same resource will return the same handle.
    /// @param type The resource type.
    /// @param path The file path.
    /// @param options Optional optins that **must** be corresponding to the loader (e.g. ModelLoaderOptions for ModelLoader).
    /// @throws InvalidResourceLoaderError If the loader for this type is not registered.
    /// @return A handle to the loaded resource, invalid if loading failed.
    /// IMPORTANT: ILoaderOptions must be deleted as the actual derived class, not as the base class. @see ILoaderOptions.
    ResourceHandle loadFromFile(ResourceType type, std::string_view path, LoaderOptions_t options = std::nullopt);

    /// @brief Load a resource of a specific type from memory.
    /// @param type The resource type.
    /// @param data A pointer to the resource data to load.
    /// @param size The size of the @p data.
    /// @param options Optional optins that **must** be corresponding to the loader (e.g. ModelLoaderOptions for ModelLoader).
    /// @throws InvalidResourceLoaderError If the loader for this type is not registered.
    /// @return A handle to the loaded resource, invalid if loading failed.
    /// IMPORTANT: ILoaderOptions must be deleted as the actual derived class, not as the base class. @see ILoaderOptions.
    ResourceHandle loadFromMemory(ResourceType type, void const *data, size_t size, LoaderOptions_t options = std::nullopt);

    /// @brief Load an already loaded resrouce in the manager.
    /// @param type the resource type.
    /// @param resource The resource.
    /// @return A handle to the resource. Invalid if @p resource is nullptr.
    ResourceHandle addRawResource(ResourceType type, std::unique_ptr<IResource> &&resource);

    /// @brief Get a resource from handle.
    /// @tparam Resource The type of a resource.
    /// @return An optional reference to the resource. Nullopt if the handle is invalid. Cast it yourself.
    ResourceRes_t getResource(ResourceHandle const &handle) const;

    /// @brief Add a loader for a specific type.
    /// Overrides if a loader of a same type is already added.
    /// @param type The type of the loader.
    /// @param loader The unique pointer to the loader.
    void registerLoader(ResourceType type, std::unique_ptr<ILoader> &&loader);

    /// @brief Run garbage collection and delete resources with no handles referencing them.
    void doGarbageCollection();
};

} // namespace engine
