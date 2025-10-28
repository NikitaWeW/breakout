#pragma once
#include "spdlog/spdlog.h"
#include <memory>

// output glm objects with spdlog
#include "fmt/ostream.h"
#define GLM_ENABLE_EXPERIMENTAL
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


#ifdef ENGINE_NO_OUTPUT

#define ENGINE_CORE_TRACE(...)
#define ENGINE_CORE_INFO(...)
#define ENGINE_CORE_WARN(...)
#define ENGINE_CORE_ERROR(...)
#define ENGINE_CORE_CRITICAL(...)

#define ENGINE_TRACE(...)
#define ENGINE_INFO(...)
#define ENGINE_WARN(...)
#define ENGINE_ERROR(...)
#define ENGINE_CRITICAL(...)

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
