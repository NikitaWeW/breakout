#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#define PROFILER_PROFILE_IN_RELEASE
#include "core/profiler.hpp"
#include <ostream>
#include <streambuf>

/*! \cond Doxygen_Suppress */

static profiler::Logger g_logger{"build/benchmark_profiler_log"};

TEST_CASE("Baseline: empty lambda call", "[baseline][benchmark]") {
    auto empty = [](){};
    BENCHMARK("empty lambda") {
        empty();
        return 0;
    };
}

TEST_CASE("Direct push/pop calls", "[profiler][benchmark]") {
    BENCHMARK("Logger::push + Logger::pop") {
        g_logger.push(profiler::Event{
            .type = profiler::Event::ENTER,
            .timestamp = profiler::timestamp(),
            .id = 1,
            .pid = 1,
            .name = "benchmark"
        });
        g_logger.push(profiler::Event{
            .type = profiler::Event::EXIT,
            .timestamp = profiler::timestamp(),
            .id = 1,
            .pid = 1,
            .name = "benchmark"
        });
        return 0;
    };
}

TEST_CASE("ScopedTimer ctor + dtor", "[profiler][benchmark]") {
    BENCHMARK("ScopedTimer ctor+dtor") {
        profiler::ScopedTimer timer{"benchmark_scope", &g_logger};
        return 0;
    };
}

TEST_CASE("PROFILER_PROFILE() macro", "[profiler][benchmark]") {
    BENCHMARK("PROFILER_PROFILE") {
        PROFILER_PROFILE();
        return 0;
    };
}

TEST_CASE("Bulk creation: 1,000 ScopedTimers", "[profiler][benchmark]") {
    BENCHMARK("1,000 ScopedTimer instances") {
        for(int i = 0; i < 1000; ++i) {
            profiler::ScopedTimer timer{"bulk_scope", &g_logger};
        }
        return 0;
    };
}

/*! \endcond */