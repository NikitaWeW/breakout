#include "config.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> engine::Logger::s_engineLogger = {};
std::shared_ptr<spdlog::logger> engine::Logger::s_appLogger = {};
void engine::Logger::init() 
{
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::info);
#endif
    spdlog::set_pattern("%^[%T][%n][%l]: %v%$");
    s_engineLogger = spdlog::stdout_color_mt("ENGINE");
    s_appLogger = spdlog::stdout_color_mt("APP");
}