#pragma once
#include "engine/config.hpp"
#include "ecs.hpp"
#include "engine/data.hpp"

namespace engine
{
    namespace detail
    {
        class ILoader
        {
        public:
            ILoader() = default;
            virtual ~ILoader() = default;
            virtual ecs::entity load(ecs::registry &reg, std::string_view path) = 0;
        };
    } // namespace detail
    
    class Loader
    {
    private:
        std::vector<std::unique_ptr<detail::ILoader>> m_loaders;
        std::unordered_map<DataType, detail::ILoader *> m_loaderMap;
        ecs::registry *m_registry;
    public:
        Loader() = default;
        Loader(ecs::registry &registry);
        ~Loader() = default;

        Loader(Loader const &) = delete;
        Loader &operator=(Loader const &) = delete;

        ecs::entity load(DataType type, std::string_view path);
    };
} // namespace engine
