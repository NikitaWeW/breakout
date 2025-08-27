/*
                      __ _ _           
     _ __  _ __ ___  / _(_) | ___ _ __ 
    | '_ \| '__/ _ \| |_| | |/ _ \ '__|    Copyright (c) 2024 Nikita Martynau
    | |_) | | | (_) |  _| | |  __/ |       https://opensource.org/license/mit
    | .__/|_|  \___/|_| |_|_|\___|_|       <TODO: insert repo name here>
    |_|                                

Instrumental profiler with scoped timers to measure performance and find bottlenecks. 
To use it, add PROFILER_PROFILE(), at the beginning of the 
function you want to profile.
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
#include <stdexcept>
#include <filesystem>
#include <mutex>
#include <future>
#include "MPMCQueue.h"

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

#define PROFILER_PROFILE() profiler::ScopedTimer PROFILER_CONCAT(_profiler_scope_timer_, __LINE__){BOOST_CURRENT_FUNCTION, profiler::getLogger()}
#else
#define PROFILER_PROFILE()
#endif

#define PROFILER_ASSERT(x, msg) assert((x) && (msg))
#define PROFILER_STATIC_ASSERT(x, msg) static_assert((x) && (msg))
#define PROFILER_THROW(x) (throw (x))

#define PROFILER_LOG_FILE "log.json"

/*! \endcond */


namespace profiler
{
    /**
     * \brief The clock alias.
     */
    using Clock_t = std::chrono::high_resolution_clock;
    /**
     * \brief The timepoint alias.
     */
    using Timepoint_t = std::chrono::time_point<Clock_t>;

    using Timestamp_t = long;

    /**
     * \brief The timer identifier alias.
     */
    using id_t = unsigned long;

    /**
     * \brief Get current timestamp.
     */    
    Timestamp_t timestamp();

    /**
     * \brief The profiling event.
     */
    struct Event
    {
        /**
         * \brief The type of the event.
         */
        enum : unsigned char {
            ENTER, EXIT
        } type;
        /**
         * \brief The timestamp of the event.
         */
        Timestamp_t timestamp = 0;
        /**
         * \brief The id of the event.
         */
        id_t id = 0;
        /**
         * \brief The process id of the event.
         */
        id_t pid = 0;
        /**
         * \brief The thread id of the event.
         */
        std::thread::id tid;
        /**
         * \brief The name of the scope instance.`
         */
        std::string_view name = "";
    };

    /**
     * \brief Used to output the profiling data to the file in a json format.
     */
    class Logger
    {
    private:
        std::ostream *m_output = nullptr;
        std::fstream m_file;
        bool m_isFirstPush = true;
        bool m_runWriter = true;
        rigtorp::MPMCQueue<Event> m_events{100};
        std::future<void> m_writer;

        // format functions
        void header();
        void foot();
        
        void startWriter();
        void stopWriter();
    public:
        Logger() = default;
        explicit Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        /**
         * \brief Default move constructor.
         */
        explicit Logger(Logger&&) = default;
        /**
         * \brief Default move operator.
         */
        Logger& operator=(Logger&&) = default;
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

        /**
         * \brief Destructor.
         */
        ~Logger();

        /**
         * \brief Prints an event to the output stream.
         * \param event The event to print.
         * \note This is not correct way to log profiling data. Use push to add an event.
         */
        void print(Event const &event);

        /**
         * \brief Pushes the event to register.
         * \param event The event to print.
         */
        void push(Event const &event);

        /**
         * \copydoc push
         */
        void push(Event &&event);

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
    class ScopedTimer 
    {
    private:
        Logger *m_logger;
        std::string_view m_name;
        id_t m_id;
        static std::atomic_ullong nextID;
    public:
        ScopedTimer() = default;
        /**
         * \param name The name of the timer. Usually the name of the function.
         * \param logger An pointer to the logger to write to.
         */
        explicit ScopedTimer(std::string_view name, Logger *logger);
        ~ScopedTimer();

        explicit ScopedTimer(ScopedTimer const &other) = delete;
        explicit ScopedTimer(ScopedTimer &&other) = delete;

        ScopedTimer &operator=(ScopedTimer const &other) = delete;
        ScopedTimer &operator=(ScopedTimer &&other) = delete;
    };

    /**
     * \brief Singleton getter.
     * \return The pointer to the requested logger.
     */
    inline Logger *getLogger() {
        static Logger logger{PROFILER_LOG_FILE};
        return &logger;
    }
} // namespace profiler

// ===============================================================|
// ===============================================================|

// Current format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview?tab=t.0

inline void profiler::Logger::header()
{
    *m_output 
    << "{\n"
    << "\t\"displayTimeUnit\": \"ns\",\n"
    << "\t\"traceEvents\": [";
}

inline void profiler::Logger::print(Event const &event)
{
    if(!m_isFirstPush)
        *m_output << ',';
    *m_output 
    << "\n\t\t{\n"
    << "\t\t\t\"ts\":        "   << event.timestamp                          << ",\n"
    << "\t\t\t\"ph\":        \"" << (event.type == Event::ENTER ? "B" : "E") << "\",\n"
    << "\t\t\t\"tid\":       "   << event.tid                                << ",\n"
    << "\t\t\t\"pid\":       "   << event.pid                                << ",\n"
    << "\t\t\t\"id\":        "   << event.id                                 << ",\n"
    << "\t\t\t\"name\":      \"" << event.name                               << "\",\n"
    << "\t\t\t\"cat\":       \""    "profiler"                                  "\"\n"
    << "\t\t}";

    m_isFirstPush = false;
}

inline void profiler::Logger::foot()
{
    *m_output << "\n\t]\n}\n";
}

inline long profiler::timestamp()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock_t::now().time_since_epoch()).count();
}

// ===============================================================|
// ===============================================================|

inline void profiler::Logger::startWriter()
{
    m_writer = std::async(std::launch::async, std::function{[this](){
        Event event{};
        while(m_runWriter)
        {
            if(!m_events.try_pop(event))
                continue;

            print(event);
        }
    }});
}

inline void profiler::Logger::stopWriter()
{
    m_runWriter = false;
    m_writer.wait();
}

inline profiler::Logger::Logger(std::filesystem::path const &filename)
{
    if(filename.has_parent_path())
        std::filesystem::create_directories(filename.parent_path());
    m_file.open(filename, std::fstream::out | std::fstream::trunc);
    if(!m_file) {
        PROFILER_THROW(std::runtime_error{"failed to open file " + filename.string()});
    }
    m_output = &m_file;
    header();
    startWriter();
}

inline profiler::Logger::Logger(std::ostream &stream) : m_output(&stream) 
{
    header();
    startWriter();
}

inline profiler::Logger::~Logger()
{
    stopWriter();
    foot();
    m_output->flush();
}

inline void profiler::Logger::push(Event const &event)
{
    m_events.push(event);
}
inline void profiler::Logger::push(Event &&event)
{
    m_events.push(std::move(event));
}

inline void profiler::Logger::clear() 
{
    m_output->clear();
}

inline profiler::ScopedTimer::ScopedTimer(std::string_view name, Logger *logger) : m_logger(logger), m_name(name), m_id(nextID++)
{
    m_logger->push(Event{
        .type = Event::ENTER,
        .timestamp = timestamp(),
        .id = m_id,
        .pid = 1,
        .tid = std::this_thread::get_id(),
        .name = m_name
    });
}
inline profiler::ScopedTimer::~ScopedTimer()
{
    m_logger->push(Event{
        .type = Event::EXIT,
        .timestamp = timestamp(),
        .id = m_id,
        .pid = 1,
        .tid = std::this_thread::get_id(),
        .name = m_name
    });
}
inline std::atomic_ullong profiler::ScopedTimer::nextID = 0;
