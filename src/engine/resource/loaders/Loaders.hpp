#pragma once
#include "engine/Resource/ResourceManager.hpp"
#include "engine/Resource/Resources.hpp"
#include "engine/Logging/Logging.hpp"

namespace engine
{

class TextureLoader : public ILoader
{
public:
    std::unique_ptr<IResource> loadFromFile(std::string_view path, LoaderOptions_t options = std::nullopt) override;
    std::unique_ptr<IResource> loadFromMemory(void const *data, size_t size, LoaderOptions_t options = std::nullopt) override;
};
class CubemapLoader : public TextureLoader
{
    std::unique_ptr<IResource> loadFromFile(std::string_view path, LoaderOptions_t options = std::nullopt) override;
    std::unique_ptr<IResource> loadFromMemory(void const *data, size_t size, LoaderOptions_t options = std::nullopt) override;
};
class ModelLoader : public ILoader, Handle<struct ModelLoaderImpl>
{
    std::unique_ptr<IResource> loadFromFile(std::string_view path, LoaderOptions_t options = std::nullopt) override;
    std::unique_ptr<IResource> loadFromMemory(void const *data, size_t size, LoaderOptions_t options = std::nullopt) override;
};

template<typename T>
static T castOptions(LoaderOptions_t options)
{
    ENGINE_ASSERT_MSG(dynamic_cast<T const *>(&options.value().get()), "Invalid options class!");
    return options.has_value() ? static_cast<T const &>(options.value().get()) : T{};
}
template<typename T>
static std::unique_ptr<T> castResource(std::unique_ptr<IResource> &&res)
{
    return std::unique_ptr<T>{static_cast<T *>(res.release())}
}

} // namespace engine::loader
