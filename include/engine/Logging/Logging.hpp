#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ranges.h"
#include <memory>
#include "engine/Header/Config.hpp"

// output glm objects with spdlog
#include "fmt/ostream.h"
#include "glm/gtx/io.hpp"

namespace engine
{
    class Logger
    {
    private:
        inline static std::shared_ptr<spdlog::logger> s_engineLogger;
        inline static std::shared_ptr<spdlog::logger> s_appLogger;
    public:
        static void init();
        inline static std::shared_ptr<spdlog::logger> &getEngineLogger() { return s_engineLogger; };
        inline static std::shared_ptr<spdlog::logger> &getAppLogger() { return s_appLogger; };
    };
}


#ifdef ENGINE_NO_LOG

#define ENGINE_CORE_TRACE(...) (void)0
#define ENGINE_CORE_INFO(...) (void)0
#define ENGINE_CORE_WARN(...) (void)0
#define ENGINE_CORE_ERROR(...) (void)0
#define ENGINE_CORE_CRITICAL(...) (void)0

#define ENGINE_TRACE(...) (void)0
#define ENGINE_INFO(...) (void)0
#define ENGINE_WARN(...) (void)0
#define ENGINE_ERROR(...) (void)0
#define ENGINE_CRITICAL(...) (void)0

#else

#define ENGINE_CORE_TRACE(...)    ::engine::Logger::getEngineLogger()->trace(__VA_ARGS__)
#define ENGINE_CORE_INFO(...)     ::engine::Logger::getEngineLogger()->info(__VA_ARGS__)
#define ENGINE_CORE_WARN(...)     ::engine::Logger::getEngineLogger()->warn(__VA_ARGS__)
#define ENGINE_CORE_ERROR(...)    ::engine::Logger::getEngineLogger()->error(__VA_ARGS__)
#define ENGINE_CORE_CRITICAL(...) ::engine::Logger::getEngineLogger()->critical(__VA_ARGS__)

#define ENGINE_TRACE(...)         ::engine::Logger::getAppLogger()->trace(__VA_ARGS__)
#define ENGINE_INFO(...)          ::engine::Logger::getAppLogger()->info(__VA_ARGS__)
#define ENGINE_WARN(...)          ::engine::Logger::getAppLogger()->warn(__VA_ARGS__)
#define ENGINE_ERROR(...)         ::engine::Logger::getAppLogger()->error(__VA_ARGS__)
#define ENGINE_CRITICAL(...)      ::engine::Logger::getAppLogger()->critical(__VA_ARGS__)

#endif
