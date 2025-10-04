#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "engine/data.hpp"

namespace engine
{
    enum LoadingFlags : int
    {
        NONE = 0,
        MODEL_FLIP_TEXTURES = 1 << 0,
        MODEL_FLIP_WINDING_ORDER = 1 << 1,
    };
    
    namespace detail
    {
        class ILoader
        {
        public:
            ILoader() = default;
            virtual ~ILoader() = default;
            /**
             * \brief Load from file into the ecs::registry.
             */
            virtual ecs::entity load(ecs::registry &reg, std::string_view path, LoadingFlags flags) 
            { 
                ENGINE_CORE_ERROR("Calling undefined ILoader function: {}", "load(ecs::registry &reg, std::string_view path, LoadingFlags flags)"); 
                return 0;
            }
            /**
             * \brief Load from file into the ecs::registry.
             */
            virtual ecs::entity load(ecs::registry &reg, std::size_t size, void const *data, LoadingFlags flags) 
            { 
                ENGINE_CORE_ERROR("Calling undefined ILoader function: {}", "load(ecs::registry &reg, void const *data, std::size_t size, LoadingFlags flags)"); 
                return 0;
            }
        };
    } // namespace detail

    class Loader
    {
    private:
        std::vector<std::unique_ptr<detail::ILoader>> m_loaders;
        std::unordered_map<DataType, detail::ILoader *> m_loaderMap;
        ecs::registry *m_registry;

        bool checkType(DataType type);
    public:
        Loader() = default;
        Loader(ecs::registry &registry);
        ~Loader() = default;

        Loader(Loader const &) = delete;
        Loader &operator=(Loader const &) = delete;

        ecs::entity load(DataType type, std::string_view path, LoadingFlags flats = LoadingFlags::NONE);
        ecs::entity load(DataType type, std::size_t size, void const *data, LoadingFlags flags = LoadingFlags::NONE);

        void registerLoader(DataType type, std::unique_ptr<detail::ILoader> &&loader);
    };
} // namespace engine

inline void ::engine::Loader::registerLoader(DataType type, std::unique_ptr<detail::ILoader> &&loader)
{
    m_loaders.emplace_back(std::move(loader));
    m_loaderMap[type] = m_loaders.back().get();
}
