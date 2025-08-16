#define CATCH_CONFIG_MAIN
#include "catch2/catch_test_macros.hpp"
#include "core/profiler.hpp"
#include <thread>
#include <algorithm>

using namespace profiler;
using namespace std::chrono_literals;

static std::chrono::nanoseconds two_ms_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(2ms);
}

TEST_CASE("getLogger returns same/shared_ptr instance", "[profiler][getLogger]") {
    auto A1 = getLogger<LogType::READABLE>("build/foo");
    auto A2 = getLogger<LogType::READABLE>("build/foo");
    auto B  = getLogger<LogType::READABLE>("build/bar");

    REQUIRE(A1 == A2);
    REQUIRE(A1 != B);
}

TEST_CASE("Logger<READABLE> push/pop emits ASCII tree", "[profiler][READABLE]") {
    std::ostringstream oss;
    {
        Logger<LogType::READABLE> logger{oss};
        logger.push("Root");
        logger.push("Child1");
        logger.push("Child2");
        logger.pop(two_ms_ns());
        logger.pop(two_ms_ns());
        logger.pop(two_ms_ns());
    }

    std::string out = oss.str();

    REQUIRE(out.find("Root") != std::string::npos);
    REQUIRE(out.find("Child1") != std::string::npos);
    REQUIRE(out.find("Child2") != std::string::npos);
    REQUIRE(out.find("finished in") != std::string::npos);

    REQUIRE(out.find("|- Child1") != std::string::npos);
    REQUIRE(out.find("|- Child2") != std::string::npos);
    REQUIRE(out.find("|= finished") != std::string::npos);
}

TEST_CASE("Logger<MARKDOWN> push/pop emits Markdown list with colors", "[profiler][MARKDOWN]") {
    std::ostringstream oss;
    {
        Logger<LogType::MARKDOWN> logger{oss};
        logger.push("A");
        logger.push("B");
        logger.pop(two_ms_ns());
        logger.pop(two_ms_ns());
    }

    std::string out = oss.str();

    REQUIRE(out.find("- `A`") != std::string::npos);
    REQUIRE(out.find("  - `B`") != std::string::npos);

    REQUIRE(out.find("<span style=\"color:") != std::string::npos);
    REQUIRE(out.find("finished in")          != std::string::npos);
}

TEST_CASE("Logger<JSON> push/pop builds valid JSON tree", "[profiler][JSON]") {
    std::ostringstream oss;
    {
        Logger<LogType::JSON> logger{oss};
        logger.push("X");
        logger.push("Y");
        logger.pop(two_ms_ns());
        logger.pop(two_ms_ns());
    }

    auto out = nlohmann::json::parse(oss.str());
    // root must have a "stack" array
    REQUIRE(out.contains("stack"));
    auto stack = out["stack"];
    REQUIRE(stack.is_array());
    REQUIRE(stack.size() == 1);

    auto root = stack[0];
    REQUIRE(root["name"] == "X");
    REQUIRE(root.contains("children"));
    auto children = root["children"];
    REQUIRE(children.is_array());
    REQUIRE(children.size() == 1);

    auto leaf = children[0];
    REQUIRE(leaf["name"] == "Y");
    REQUIRE(leaf["finished in (ms)"].is_number());
    REQUIRE(leaf["children"].is_array());
    REQUIRE(leaf["children"].empty());
}

TEST_CASE("ScopedTimer<MARKDOWN> pushes/pops automatically", "[profiler][ScopedTimer]") {
    std::ostringstream oss;

    auto logger = std::make_shared<Logger<LogType::MARKDOWN>>(oss);

    {
        ScopedTimer<LogType::MARKDOWN> t1{"Outer", logger};
        std::this_thread::sleep_for(1ms);
        {
            ScopedTimer<LogType::MARKDOWN> t2{"Inner", logger};
            std::this_thread::sleep_for(1ms);
        }
    }


    logger.reset();

    std::string out = oss.str();

    REQUIRE(out.find("- `Outer`") != std::string::npos);
    REQUIRE(out.find("  - `Inner`") != std::string::npos);
    REQUIRE(out.find("finished in") != std::string::npos);
}

TEST_CASE("Concurrent getLogger produces single instance per name", "[profiler][concurrency][getLogger]") {
    constexpr int threads_count = 16;
    constexpr int calls_per_thread = 100;
    std::vector<std::shared_ptr<Logger<LogType::READABLE>>> collected;
    collected.reserve(threads_count * calls_per_thread);

    std::vector<std::thread> threads;
    threads.reserve(threads_count);

    // names to choose from
    std::vector<std::string> names = {"alpha", "beta", "gamma"};

    std::mutex collect_mtx;
    for (int t = 0; t < threads_count; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < calls_per_thread; ++i) {
                auto name = names[i % names.size()];
                auto logger = getLogger<LogType::READABLE>(name);
                std::scoped_lock lock(collect_mtx);
                collected.push_back(logger);
            }
        });
    }
    for (auto &th : threads) th.join();

    std::map<std::ostream *, std::shared_ptr<Logger<LogType::READABLE>>> unique_map;
    for (auto &ptr : collected) {
        unique_map.emplace(ptr->getOutputStream(), ptr);
    }

    REQUIRE(unique_map.size() == names.size());
}

TEST_CASE("Concurrent ScopedTimer logging on READABLE logger", "[profiler][concurrency][ScopedTimer]") {
    constexpr int thread_count = 8;
    constexpr int iterations = 200;
    std::ostringstream oss;
    auto logger = std::make_shared<Logger<LogType::READABLE>>(oss);

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < iterations; ++i) {
                ScopedTimer<LogType::READABLE> timer{"Task_" + std::to_string(t) + "_" + std::to_string(i), logger};
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    for (auto &th : threads) th.join();

    logger.reset();

    std::string out = oss.str();

    int count = 0;
    size_t pos = out.find("Task", 0);

    while (pos != std::string::npos) {
        count++;
        pos = out.find("Task", pos + 1);
    }


    REQUIRE(count == thread_count * iterations);
}