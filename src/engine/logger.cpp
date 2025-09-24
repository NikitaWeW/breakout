#include "config.hpp"

std::shared_ptr<spdlog::logger> engine::Logger::s_engineLogger = {};
std::shared_ptr<spdlog::logger> engine::Logger::s_appLogger = {};
void engine::Logger::init() 
{
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_engineLogger = spdlog::stdout_color_mt("Engine");
    s_appLogger = spdlog::stdout_color_mt("Application");
}