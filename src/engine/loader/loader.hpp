#pragma once
#include "engine/core/ecs.hpp"
#include "engine/core/data.hpp"
#include "engine/core/logging.hpp"

namespace engine
{
    enum class LoadingFlags : int
    {
        NONE = 0,
        MODEL_FLIP_TEXTURES = 1 << 0,
        MODEL_FLIP_WINDING_ORDER = 1 << 1,
        FLIP_TEXTURES = 1 << 2,
    };

    constexpr LoadingFlags DEFAULT_LOADING_FLAGS = LoadingFlags::MODEL_FLIP_TEXTURES;
    
    namespace loader
    {
        class ILoader
        {
        public:
            ILoader() = default;
            virtual ~ILoader() = default;
            /// \brief Load from file into the ecs::registry.
            virtual inline ecs::entity load(ecs::registry &, std::string_view, LoadingFlags) 
            { 
                ENGINE_CORE_ERROR("Calling undefined ILoader function: {}", "load(ecs::registry &reg, std::string_view path, LoadingFlags flags)"); 
                return 0;
            }
            /// \copydoc load
            virtual inline ecs::entity load(ecs::registry &, std::size_t, void const *, LoadingFlags) 
            { 
                ENGINE_CORE_ERROR("Calling undefined ILoader function: {}", "load(ecs::registry &reg, void const *data, std::size_t size, LoadingFlags flags)"); 
                return 0;
            }
        };
    } // namespace loader

    class Loader
    {
    private:
        std::vector<std::unique_ptr<loader::ILoader>> m_loaders;
        std::unordered_map<DataType, loader::ILoader *> m_loaderMap;
        ecs::registry *m_registry;

        bool checkType(DataType type);
    public:
        Loader() = default;
        Loader(ecs::registry &registry);
        ~Loader() = default;

        Loader(Loader const &) = delete;
        Loader &operator=(Loader const &) = delete;

        ecs::entity load(DataType type, std::string_view path, LoadingFlags flats = DEFAULT_LOADING_FLAGS);
        ecs::entity load(DataType type, std::size_t size, void const *data, LoadingFlags flags = DEFAULT_LOADING_FLAGS);

        void registerLoader(DataType type, std::unique_ptr<loader::ILoader> &&loader);
    };
} // namespace engine

inline void ::engine::Loader::registerLoader(DataType type, std::unique_ptr<loader::ILoader> &&loader)
{
    m_loaders.emplace_back(std::move(loader));
    m_loaderMap[type] = m_loaders.back().get();
}

// For some reason one cant access bitwise operators in the scoped enums. Annoying.
inline constexpr engine::LoadingFlags operator|(engine::LoadingFlags lhs, engine::LoadingFlags rhs) {
    return static_cast<engine::LoadingFlags>(
        static_cast<std::underlying_type_t<engine::LoadingFlags>>(lhs) |
        static_cast<std::underlying_type_t<engine::LoadingFlags>>(rhs)
    );
}
inline constexpr engine::LoadingFlags operator&(engine::LoadingFlags lhs, engine::LoadingFlags rhs) {
    return static_cast<engine::LoadingFlags>(
        static_cast<std::underlying_type_t<engine::LoadingFlags>>(lhs) &
        static_cast<std::underlying_type_t<engine::LoadingFlags>>(rhs)
    );
}
inline constexpr engine::LoadingFlags operator~(engine::LoadingFlags p) {
    return static_cast<engine::LoadingFlags>(
        ~static_cast<std::underlying_type_t<engine::LoadingFlags>>(p)
    );
}
