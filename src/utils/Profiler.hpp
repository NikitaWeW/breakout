#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include "json.hpp"
#include "current_function.hpp"

#ifndef DEBUG
#define PROFILER_LOG_TYPE profiler::READABLE
#define PROFILER_PROFILE() profiler::ScopeTimer<PROFILER_LOG_TYPE> profiler_scope_timer_ ## BOOST_CURRENT_FUNCTION{BOOST_CURRENT_FUNCTION, profiler::getLogger<PROFILER_LOG_TYPE>("log/profiler.txt")}
#define PROFILER_PROFILE_IN_FILE(filepath) profiler::ScopeTimer<PROFILER_LOG_TYPE> profiler_scope_timer_ ## BOOST_CURRENT_FUNCTION{BOOST_CURRENT_FUNCTION, profiler::getLogger<PROFILER_LOG_TYPE>(filepath)}
#else
#define PROFILER_PROFILE()
#define PROFILER_PROFILE_IN_FILE(filepath)
#endif

namespace profiler
{
    enum LogType
    {
        READABLE, JSON
    };

    template <LogType> struct Log_t {};
    template <> struct Log_t<profiler::LogType::READABLE> : std::stringstream {};
    template <> struct Log_t<profiler::LogType::JSON> : nlohmann::json {
        std::vector<nlohmann::json *> entryStack;
    };

    template<LogType type = READABLE>
    class Logger
    {
    public:
    private:
        std::fstream m_file{};
        Log_t<type> m_log;
        std::vector<std::string> m_stack;
    public:
        Logger() = default;
        Logger(std::filesystem::path const &filename);
        ~Logger();

        void pushFunction(std::string_view name);
        void popFunction(std::chrono::nanoseconds const &time);

        void clear();
    };

    template <LogType type>
    class ScopeTimer 
    {
    private:
        using Clock_t = std::chrono::high_resolution_clock;
        using Timepoint_t = std::chrono::time_point<Clock_t>;
        Timepoint_t m_start;
        Logger<type> *m_logger;
    public:
        ScopeTimer() = default;
        ScopeTimer(std::string_view name, Logger<type> &logger);
        ~ScopeTimer();
    };

    // singleton getters
    template <LogType type>
    inline Logger<type> &getLogger(std::string_view name) {
        static Logger<type> logger{name};
        return logger;
    }
} // namespace profiler


template<profiler::LogType type>
inline profiler::Logger<type>::Logger(std::filesystem::path const &filename)
{
    std::filesystem::create_directories(filename.parent_path());
    m_file.open(filename, std::fstream::in | std::fstream::out | std::fstream::trunc);
    if(!m_file) {
        throw std::runtime_error{"failed to open file " + std::string{filename}};
    }
}

template <> 
inline profiler::Logger<profiler::LogType::READABLE>::~Logger()
{
    std::time_t result = std::time(nullptr);
    std::string timeDate = std::asctime(std::localtime(&result));
    timeDate.pop_back();
    m_file << "============================\n| " << timeDate << " |\n============================\n";
    m_file << m_log.rdbuf();
}
template <> 
inline profiler::Logger<profiler::LogType::JSON>::~Logger()
{
    std::time_t result = std::time(nullptr);
    std::string timeDate = std::asctime(std::localtime(&result));
    timeDate.pop_back();
    m_log["date and time"] = timeDate;
    m_file << m_log.dump(4);
}

template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::pushFunction(std::string_view name)
{
    for(size_t i = 0; i < m_stack.size(); ++i) {
        m_log << "|  ";
    }
    if(m_stack.size() != 0) {
        m_log << "|- ";
    } else {
        m_log << "\n";
    }
    m_log << name
    << '\n';
    m_stack.push_back(std::string{name});
}
template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::popFunction(std::chrono::nanoseconds const &time)
{
    for(size_t i = 1; i < m_stack.size(); ++i) {
        m_log << "|  ";
    }
    if(m_stack.size() != 0) {
        m_log << "|= ";
    }
    m_log << "finished in " << time.count() * 1e-6f << "ms"
    << '\n';
    m_stack.pop_back();
}

template <> 
inline void profiler::Logger<profiler::LogType::JSON>::pushFunction(std::string_view name)
{
    nlohmann::json entry = {
        {"name",  name},
        {"finished in (ms)", nullptr},
        {"children", nlohmann::json::array()}
    };

    if(m_log.entryStack.empty()) {
        m_log["stack"].push_back(std::move(entry));
        m_log.entryStack.push_back(&m_log["stack"].back());
    } else {
        m_log.entryStack.back()->at("children").push_back(std::move(entry));
        m_log.entryStack.push_back(&m_log.entryStack.back()->at("children").back());
    }

    m_stack.push_back(std::string{name});
}
template <> 
inline void profiler::Logger<profiler::LogType::JSON>::popFunction(std::chrono::nanoseconds const &time)
{
    m_log.entryStack.back()->at("finished in (ms)") = time.count() * 1e-6f;
    m_stack.pop_back();
    m_log.entryStack.pop_back();
}

template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::clear()
{
    m_log.str(std::string());
}
template <> 
inline void profiler::Logger<profiler::LogType::JSON>::clear()
{
    m_log.clear();
}

template <profiler::LogType type>
inline profiler::ScopeTimer<type>::ScopeTimer(std::string_view name, Logger<type> &logger)
{
    m_start = Clock_t::now();
    m_logger = &logger;
    m_logger->pushFunction(name);
}
template <profiler::LogType type>
inline profiler::ScopeTimer<type>::~ScopeTimer()
{
    m_logger->popFunction(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock_t::now() - m_start));
}
