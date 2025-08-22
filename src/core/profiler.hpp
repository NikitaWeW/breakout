/*
                      __ _ _           
     _ __  _ __ ___  / _(_) | ___ _ __ 
    | '_ \| '__/ _ \| |_| | |/ _ \ '__|    Copyright (c) 2024 Nikita Martynau
    | |_) | | | (_) |  _| | |  __/ |       https://opensource.org/license/mit
    | .__/|_|  \___/|_| |_|_|\___|_|       <todo: insert repo name here>
    |_|                                

Profiler with scoped timers to measure performance and find bottlenecks. 
To use it, add PROFILER_PROFILE(), PROFILER_PROFILE_IN_FILE(filepath), 
PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, logtype) at the beginning of the 
function you want to profile. See the profiler::LogType to see the log types available.
*/
/*
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <mutex>
#include "json.hpp"

/*! \cond Doxygen_Suppress */

#ifndef NDEBUG
//  http://www.boost.org/libs/assert/current_function.html
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  http://www.boost.org/LICENSE_1_0.txt

#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
# define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
# define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
# define BOOST_CURRENT_FUNCTION __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
# define BOOST_CURRENT_FUNCTION __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
# define BOOST_CURRENT_FUNCTION __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
# define BOOST_CURRENT_FUNCTION __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
# define BOOST_CURRENT_FUNCTION __func__
#else
# define BOOST_CURRENT_FUNCTION "(unknown)"
#endif

#define PROFILER_CONCAT_DETAIL(A, B) A##B
#define PROFILER_CONCAT(A, B) PROFILER_CONCAT_DETAIL(A, B)
#define PROFILER_UNIQUE_VAR(base) PROFILER_CONCAT(base, __LINE__)

#define PROFILER_PROFILE() PROFILER_PROFILE_IN_FILE("log/profiler")
#define PROFILER_PROFILE_IN_FILE(filepath) PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, profiler::MARKDOWN)
#define PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, logtype) profiler::ScopedTimer<logtype> PROFILER_UNIQUE_VAR(_profiler_scope_timer_){BOOST_CURRENT_FUNCTION, profiler::getLogger<logtype>(filepath)}
#else
#define PROFILER_PROFILE()
#define PROFILER_PROFILE_IN_FILE(filepath)
#endif

#define PROFILER_ASSERT(x, msg) assert((x) && (msg))
#define PROFILER_STATIC_ASSERT(x, msg) static_assert((x) && (msg))
#define PROFILER_THROW(x) (throw (x))

#define PROFILER_IMPLEMENTATION

/*! \endcond */


namespace profiler
{
    enum LogType
    {
        READABLE, MARKDOWN, JSON
    };

    /*! \cond Doxygen_Suppress */
    template <LogType> struct Log_t {};
    template <> struct Log_t<profiler::LogType::READABLE> : std::stringstream {};
    template <> struct Log_t<profiler::LogType::MARKDOWN> : std::stringstream {};
    template <> struct Log_t<profiler::LogType::JSON> : nlohmann::json {
        std::vector<nlohmann::json *> entryStack;
    };
    /*! \endcond */

    /**
     * \brief Used to output the profiling data to the file
     * \tparam type The log type.
     */
    template<LogType type = READABLE>
    class Logger
    {
    private:
        std::ostream *m_output = nullptr;
        std::fstream m_file;
        std::mutex m_logMutex;
        Log_t<type> m_log;
        std::vector<std::string> m_stack;
    public:
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        /**
         * \brief Write to a file.
         * \param filename The path to a file to write in.
         */
        Logger(std::filesystem::path const &filename);
        /**
         * \brief Write in a custom stream.
         * \param stream The stream object. Must be alive during the lifetime of a logger!!
         */
        Logger(std::ostream &stream);
        ~Logger();

        /**
         * Pushes the timer to the stack.
         * \tparam name The name of a timer.
         */
        void push(std::string_view name);

        /**
         * pops and registers the top timer with the time.
         * \param time The time of a timer.
         */
        void pop(std::chrono::nanoseconds const &time);

        /**
         * \brief Clears the log.
         */
        void clear();

        /**
         * \return The std::ostream the logger writes to.
         */
        inline std::ostream *getOutputStream() { return m_output; }
        /**
         * \copydoc getOutputStream
         */
        inline std::ostream const *getOutputStream() const { return m_output; }
    };

    /**
     * \brief The timer that starts at the construction and stops when destroyes. Outputs data to the Logger class via singleton getter.
     * \tparam type The logger type 
     */
    template <LogType type>
    class ScopedTimer 
    {
    private:
        using Clock_t = std::chrono::high_resolution_clock;
        using Timepoint_t = std::chrono::time_point<Clock_t>;
        Timepoint_t m_start;
        std::weak_ptr<Logger<type>> m_logger;
    public:
        ScopedTimer() = default;
        /**
         * \param name The name of the timer. Usally the name of the function.
         * \param logger An lvalue reference to the logger to write to.
         */
        ScopedTimer(std::string_view name, std::weak_ptr<Logger<type>> logger);
        ~ScopedTimer();
    };

    /**
     * \brief Singleton getter to get the logger for the filename easely.
     * \param name The filename.
     * \return The lvalue reference to the requested logger.
     */
    template <LogType type>
    inline std::shared_ptr<Logger<type>> getLogger(std::string_view name) {
        static std::mutex loggersMutex;
        static std::map<std::string, std::shared_ptr<Logger<type>>> loggers;
        std::scoped_lock lock{loggersMutex};

        if(loggers.find(std::string{name}) == loggers.end()) {
            loggers.try_emplace(std::string{name}, std::move(std::make_unique<Logger<type>>(name)));
        }
        return loggers.at(std::string{name});
    }
} // namespace profiler


#ifdef PROFILER_IMPLEMENTATION

template<profiler::LogType type>
inline profiler::Logger<type>::Logger(std::filesystem::path const &filename)
{
    if(filename.has_parent_path())
        std::filesystem::create_directories(filename.parent_path());
    m_file.open(filename, std::fstream::out | std::fstream::trunc);
    if(!m_file) {
        PROFILER_THROW(std::runtime_error{"failed to open file " + filename.string()});
    }
    m_output = &m_file;
}

template <profiler::LogType type>
inline profiler::Logger<type>::Logger(std::ostream &stream) : m_output(&stream)
{}

template<profiler::LogType type>
inline profiler::Logger<type>::~Logger()
{
    PROFILER_STATIC_ASSERT(type == READABLE || type == MARKDOWN || type == JSON, "log type not supported!");
}
template <> 
inline profiler::Logger<profiler::LogType::READABLE>::~Logger()
{
    PROFILER_ASSERT(m_stack.size() == 0, "not all stack frames finished!");
    std::time_t result = std::time(nullptr);
    std::string timeDate = std::asctime(std::localtime(&result));
    timeDate.pop_back();
    std::scoped_lock lock{m_logMutex};
    *m_output << "============================\n| " << timeDate << " |\n============================\n";
    *m_output << m_log.rdbuf();
}
template <> 
inline profiler::Logger<profiler::LogType::MARKDOWN>::~Logger()
{
    PROFILER_ASSERT(m_stack.size() == 0, "not all stack frames finished!");
    std::time_t result = std::time(nullptr);
    std::string timeDate = std::asctime(std::localtime(&result));
    std::scoped_lock lock{m_logMutex};
    *m_output << timeDate << "===\n\n";
    *m_output << m_log.rdbuf();
}
template <> 
inline profiler::Logger<profiler::LogType::JSON>::~Logger()
{
    PROFILER_ASSERT(m_stack.size() == 0, "not all stack frames finished!");
    std::time_t result = std::time(nullptr);
    std::string timeDate = std::asctime(std::localtime(&result));
    timeDate.pop_back();
    std::scoped_lock lock{m_logMutex};
    m_log["date and time"] = timeDate;
    *m_output << m_log.dump(4);
}

template<profiler::LogType type>
inline void profiler::Logger<type>::push(std::string_view name) 
{
    PROFILER_STATIC_ASSERT(type == READABLE || type == MARKDOWN || type == JSON, "log type not supported!");
}
template<profiler::LogType type>
inline void profiler::Logger<type>::pop(std::chrono::nanoseconds const &time)
{
    PROFILER_STATIC_ASSERT(type == READABLE || type == MARKDOWN || type == JSON, "log type not supported!");
}

template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::push(std::string_view name)
{
    std::scoped_lock lock{m_logMutex};
    if(m_stack.size() == 0) {
        m_log << "\n";
    }
    for(size_t i = 1; i < m_stack.size(); ++i) {
        m_log << "|  ";
    }
    if(m_stack.size() != 0) {
        m_log << "|- ";
    }
    m_log << name
    << '\n';
    m_stack.emplace_back(name);
}
template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::pop(std::chrono::nanoseconds const &time)
{
    std::scoped_lock lock{m_logMutex};
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
inline void profiler::Logger<profiler::LogType::MARKDOWN>::push(std::string_view name)
{
    std::scoped_lock lock{m_logMutex};
    if(m_stack.empty())
        m_log << '\n';

    m_log << std::string(m_stack.size() * 2, ' ') << "- `" << name << "`\n";

    m_stack.emplace_back(name);
}
template <>
inline void profiler::Logger<profiler::LogType::MARKDOWN>::pop(std::chrono::nanoseconds const& time)
{
    const std::size_t depth = m_stack.size();
    float timeMS = static_cast<float>(time.count()) * 1e-6f;
    std::string_view color;
    if(timeMS < 1) color = "#b5cea8";
    else if(timeMS < 5) color = "#ce9178";
    else color = "#f44747";

    std::scoped_lock lock{m_logMutex};
    m_log << std::string(depth * 2, ' ')
          << "+ **finished in** "
          << "<span style=\"color:" << color << "\">"
          << timeMS << "</span> ms\n";

    m_stack.pop_back();
}

template <> 
inline void profiler::Logger<profiler::LogType::JSON>::push(std::string_view name)
{
    nlohmann::json entry = {
        {"name",  name},
        {"finished in (ms)", nullptr},
        {"children", nlohmann::json::array()}
    };

    std::scoped_lock lock{m_logMutex};

    if(m_log.entryStack.empty()) {
        m_log["stack"].emplace_back(std::move(entry));
        m_log.entryStack.push_back(&m_log["stack"].back());
    } else {
        m_log.entryStack.back()->at("children").emplace_back(std::move(entry));
        m_log.entryStack.push_back(&m_log.entryStack.back()->at("children").back());
    }

    m_stack.emplace_back(name);
}
template <> 
inline void profiler::Logger<profiler::LogType::JSON>::pop(std::chrono::nanoseconds const &time)
{
    std::scoped_lock lock{m_logMutex};
    m_log.entryStack.back()->at("finished in (ms)") = time.count() * 1e-6f;
    m_stack.pop_back();
    m_log.entryStack.pop_back();
}

template<profiler::LogType type>
inline void profiler::Logger<type>::clear() 
{
    PROFILER_STATIC_ASSERT(type == READABLE || type == MARKDOWN || type == JSON, "log type not supported!");
}
template <> 
inline void profiler::Logger<profiler::LogType::READABLE>::clear()
{
    std::scoped_lock lock{m_logMutex};
    m_log.str(std::string());
}
template <> 
inline void profiler::Logger<profiler::LogType::MARKDOWN>::clear()
{
    std::scoped_lock lock{m_logMutex};
    m_log.str(std::string());
}
template <> 
inline void profiler::Logger<profiler::LogType::JSON>::clear()
{
    std::scoped_lock lock{m_logMutex};
    m_log.entryStack.clear();
    m_log.clear();
}

template <profiler::LogType type>
inline profiler::ScopedTimer<type>::ScopedTimer(std::string_view name, std::weak_ptr<Logger<type>> logger) : m_logger(logger)
{
    m_start = Clock_t::now();
    if(!m_logger.expired())
        m_logger.lock()->push(name);
}
template <profiler::LogType type>
inline profiler::ScopedTimer<type>::~ScopedTimer()
{
    if(!m_logger.expired())
        m_logger.lock()->pop(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock_t::now() - m_start));
}

#endif // PROFILER_IMPLEMENTATION
