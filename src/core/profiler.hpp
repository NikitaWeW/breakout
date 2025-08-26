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
#include <map>

/*! \cond Doxygen_Suppress */

#if !defined(NDEBUG) || defined(PROFILER_PROFILE_IN_RELEASE)
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
#define PROFILER_PROFILE_IN_FILE(filepath) PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, profiler::LogType::MARKDOWN)
#define PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, logtype) profiler::ScopedTimer<logtype> PROFILER_UNIQUE_VAR(_profiler_scope_timer_){BOOST_CURRENT_FUNCTION, profiler::getLogger<logtype>(filepath)}
#else
#define PROFILER_PROFILE()
#define PROFILER_PROFILE_IN_FILE(filepath)
#define PROFILER_PROFILE_IN_FILE_LOG_TYPE(filepath, logtype)
#endif

#define PROFILER_ASSERT(x, msg) assert((x) && (msg))
#define PROFILER_STATIC_ASSERT(x, msg) static_assert((x) && (msg))
#define PROFILER_THROW(x) (throw (x))

/*! \endcond */


namespace profiler
{
    using Clock_t = std::chrono::high_resolution_clock;
    using Timepoint_t = std::chrono::time_point<Clock_t>;

    /**
     * \brief The type of logging.
     * 
     */
    enum class LogType
    {
        /**
         * \brief Outputs in a markdown format.
         */
        MARKDOWN, 
        /**
         * \brief Outputs in a NDJSON format.
         * 
         * | Field name |    Type    |                             Description                              |
         * | ---------- | ---------- | -------------------------------------------------------------------- |
         * | timestamp  | uint       | Timestamp in nanoseconds                                             |
         * | event      | string     | Either enter or exit                                                 |
         * | id         | uint       | Unique identifier for this scope instance                            |
         * | name       | string     | A name of the function                                               |
         * | duration   | uint       | Duration of the scope instance in milliseconds. Only for exit event. |
         */
        JSON
    };

    /**
     * \brief Used to output the profiling data to the file
     * \tparam type The log type.
     */
    template<LogType type>
    class Logger
    {
    private:
        std::ostream *m_output = nullptr;
        std::fstream m_file;
        std::mutex m_logMutex;
        std::size_t m_nextID = 0;
        std::vector<std::string_view> m_stack;
    public:
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        /**
         * \brief Write to a file.
         * \param filename The path to a file to write in.
         */
        explicit Logger(std::filesystem::path const &filename);
        /**
         * \brief Write in a custom stream.
         * \param stream The stream object. Must be alive during the lifetime of a logger!!
         */
        explicit Logger(std::ostream &stream);
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
     * \brief The timer that starts at the construction and stops when destroys. Outputs data to the Logger class via singleton getter.
     * \tparam type The logger type 
     */
    template <LogType type>
    class ScopedTimer 
    {
    private:
        Timepoint_t m_start;
        std::weak_ptr<Logger<type>> m_logger;
    public:
        ScopedTimer() = default;
        /**
         * \param name The name of the timer. Usually the name of the function.
         * \param logger An lvalue reference to the logger to write to.
         */
        explicit ScopedTimer(std::string_view name, std::weak_ptr<Logger<type>> logger);
        ~ScopedTimer();
    };

    /**
     * \brief Singleton getter to get the logger for the filename easily.
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
inline profiler::Logger<type>::Logger(std::ostream &stream) : m_output(&stream) {}

template<profiler::LogType type>
inline profiler::Logger<type>::~Logger()
{
    PROFILER_STATIC_ASSERT(type == LogType::MARKDOWN || type == LogType::JSON, "log type not supported!");
    m_output->flush();
}

template <>
inline void profiler::Logger<profiler::LogType::MARKDOWN>::push(std::string_view name)
{
    std::scoped_lock lock{m_logMutex};
    if(m_stack.empty())
        *m_output << '\n';

    *m_output << m_nextID << ": " << std::string(m_stack.size() * 2, ' ') << "- `" << name << "`\n";

    m_stack.emplace_back(name);
    ++m_nextID;
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
    *m_output 
        << m_nextID << ": " 
         << std::string(depth * 2, ' ')
         << "+ **finished in** "
         << "<span style=\"color:" << color << "\">"
         << timeMS << "</span> ms\n";

    m_stack.pop_back();
    ++m_nextID;
}

template <> 
inline void profiler::Logger<profiler::LogType::JSON>::push(std::string_view name)
{
    std::scoped_lock lock{m_logMutex};

    *m_output 
    << "{"
    << "\n\t\"timestamp\": " << std::chrono::time_point_cast<std::chrono::nanoseconds>(Clock_t::now()).time_since_epoch().count()
    << "\n\t\"event\":     " << "\"enter\""
    << "\n\t\"id\":        " << m_nextID
    << "\n\t\"name\":      " << name
    << "\n}\n";
    
    m_stack.emplace_back(name);
    ++m_nextID;
}
template <> 
inline void profiler::Logger<profiler::LogType::JSON>::pop(std::chrono::nanoseconds const &time)
{
    std::scoped_lock lock{m_logMutex};

    *m_output 
    << "{"
    << "\n\t\"timestamp\": " << std::chrono::time_point_cast<std::chrono::nanoseconds>(Clock_t::now()).time_since_epoch().count()
    << "\n\t\"event\":     " << "\"exit\""
    << "\n\t\"id\":        " << m_nextID
    << "\n\t\"name\":      " << m_stack.front()
    << "\n\t\"duration\"   " << static_cast<float>(time.count()) * 1e-6f
    << "\n}\n";
    
    m_stack.pop_back();
    ++m_nextID;
}

template<profiler::LogType type>
inline void profiler::Logger<type>::clear() 
{
    PROFILER_STATIC_ASSERT(type == LogType::MARKDOWN || type == LogType::JSON, "log type not supported!");
    m_output->clear();
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
