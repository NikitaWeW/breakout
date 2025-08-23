#include "catch2/catch_test_macros.hpp"
#include "catch2/benchmark/catch_benchmark.hpp"
#include "core/ecs.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include "entt/entt.hpp"

/*! \cond Doxygen_Suppress */

struct Position { float  x = 0,  y = 0; };
struct Velocity { float dx = 0, dy = 0; };
struct Health   { float hp = 100; };

TEST_CASE("Create and validate entities", "[entity]") {
    ecs::registry reg;

    // Initially no entities
    REQUIRE(reg.getEntities().empty());

    // Create two empty entities
    auto e1 = reg.create<>();
    auto e2 = reg.create<>();

    // Both should be valid and distinct
    REQUIRE(reg.valid(e1));
    REQUIRE(reg.valid(e2));
    REQUIRE(e1 != e2);

    // getEntities returns both
    auto all = reg.getEntities();
    REQUIRE(all.size() == 2);
    REQUIRE(std::find(all.begin(), all.end(), e1) != all.end());
    REQUIRE(std::find(all.begin(), all.end(), e2) != all.end());
}
TEST_CASE("Add, has and get components", "[component]") {
    ecs::registry reg;
    auto e = reg.create<>();

    // Initially, no Position component
    REQUIRE_FALSE(reg.has<Position>(e));

    // Add and verify
    reg.add<Position>(e, Position{1.0f, 2.0f});
    REQUIRE(reg.has<Position>(e));

    // Retrieve via get (const) and lock (mutable)
    {
        auto constPos = reg.get<Position>(e);
        REQUIRE(constPos->x == 1.0f);
        REQUIRE(constPos->y == 2.0f);
    }

    {
        auto mutPos = reg.lock<Position>(e);
        mutPos->x = 5.0f;
    }
    REQUIRE(reg.get<Position>(e)->x == 5.0f);
}
TEST_CASE("Remove components", "[component]") {
    ecs::registry reg;
    auto e = reg.create<>();

    reg.add<Velocity>(e, Velocity{0.1f, 0.2f});
    REQUIRE(reg.has<Velocity>(e));

    reg.remove<Velocity>(e);
    REQUIRE_FALSE(reg.has<Velocity>(e));

    // Removing again should throw out_of_range
    REQUIRE_THROWS_AS(reg.remove<Velocity>(e), std::out_of_range);
}
TEST_CASE("View entities by component signature", "[view]") {
    ecs::registry reg;

    // Create 3 entities with different combos
    auto a = reg.create<Position>();
    auto b = reg.create<Position, Velocity>();
    auto c = reg.create<Velocity>();

    {
        // View entities that have Position
        auto posOnly = reg.view<Position>();
        REQUIRE(posOnly->size() == 2);
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), a) != posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), b) != posOnly.end());
    }
    {
        // View entities that have Position but exclude Velocity
        auto purePos = reg.view<Position>(ecs::exclude_t<Velocity>{});
        REQUIRE(purePos->size() == 1);
        REQUIRE(purePos->front() == a);
    }
    {
        // View only Velocity
        auto velOnly = reg.view<Velocity>();
        REQUIRE(velOnly->size() == 2);
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), b) != velOnly.end());
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), c) != velOnly.end());
    }
}
TEST_CASE("Destroy entity and its components", "[destroy]") {
    ecs::registry reg;
    auto e = reg.create<Position, Velocity>();

    REQUIRE(reg.valid(e));
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));

    reg.destroy(e);
    REQUIRE_FALSE(reg.valid(e));

    // After destroy, getSignature or has should throw invalid_argument
    REQUIRE_THROWS_AS(reg.getSignature(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.has<Position>(e), std::invalid_argument);
}
TEST_CASE("Invalid handles and double-add errors", "[errors]") {
    ecs::registry reg;
    ecs::entity bad = ecs::MAX_ENTITIES + 10;

    // invalid entity
    REQUIRE_THROWS_AS(reg.has<Position>(bad), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.add<Position>(bad, Position{}), std::invalid_argument);

    auto e = reg.create<>();
    // double add same component
    reg.add<Position>(e, Position{});
    REQUIRE_THROWS_AS(reg.add<Position>(e, Position{}), std::invalid_argument);
}

TEST_CASE("Basic entity creation and destruction", "[entity]") {
    ecs::registry registry;

    SECTION("Create and validate") {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
    }

    SECTION("Destroy and invalidate") {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
        registry.destroy(e);
        REQUIRE_FALSE(registry.valid(e));
    }

    SECTION("Destroy invalid entity throws") {
        REQUIRE_THROWS_AS(registry.destroy(9999u), std::invalid_argument);
    }
}

TEST_CASE("Component add / remove / access", "[component]") {
    ecs::registry registry;
    auto e = registry.create<>();
    
    SECTION("Initially no component") {
        REQUIRE_FALSE(registry.has<Position>(e));
    }

    SECTION("Add, has, get, modify, remove") {
        registry.add<Position>(e, Position{1.5f, 2.5f});
        REQUIRE( registry.has<Position>(e) );

        {
            // shared get
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read->x == 1.5f );
            REQUIRE( p_read->y == 2.5f );
        }

        {
            // unique lock and modify
            auto p_write = registry.lock<Position>(e);
            p_write->x = 9.0f;
            p_write->y = -3.0f;
        }
        {
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read->x == 9.0f );
            REQUIRE( p_read->y == -3.0f );
        }

        // remove and recheck
        registry.remove<Position>(e);
        REQUIRE_FALSE(registry.has<Position>(e));
        REQUIRE_THROWS_AS(registry.get<Position>(e), std::out_of_range);
    }

    SECTION("Adding duplicate component throws") {
        registry.add<Position>(e, Position{0,0});
        REQUIRE_THROWS_AS(registry.add<Position>(e, Position{0,0}), std::invalid_argument);
    }

    SECTION("Removing non-existent component throws") {
        REQUIRE_THROWS_AS(registry.remove<Velocity>(e), std::out_of_range);
    }

    SECTION("Access on invalid entity throws") {
        REQUIRE_THROWS_AS(registry.get<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.lock<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.has<Position>(999u), std::invalid_argument);
    }
}

TEST_CASE("Views: include and exclude semantics", "[view]") {
    ecs::registry registry;

    auto e1 = registry.create<Position>(Position{1,1});
    auto e2 = registry.create<Position>(Position{2,2});
    auto e3 = registry.create<Position, Velocity>(Position{3,3}, Velocity{0.5f,0.5f});
    auto e4 = registry.create<Velocity>(Velocity{9,9});

    SECTION("Include only Position") {
        auto viewPos = registry.view<Position>();
        auto entities = viewPos.get();
        std::vector<ecs::entity> got(entities.begin(), entities.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2, e3} );
    }

    SECTION("Include Position, exclude Velocity") {
        auto viewPosNoVel = registry.view<Position>(ecs::exclude_t<Velocity>{});
        auto entities = viewPosNoVel.get();
        std::vector<ecs::entity> got(entities.begin(), entities.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2} );
    }

    SECTION("Include both Position and Velocity") {
        auto viewPosVel = registry.view<Position, Velocity>();
        auto entities = viewPosVel.get();
        REQUIRE( entities.size() == 1 );
        REQUIRE( entities.front() == e3 );
    }

    SECTION("Exclude Position only Velocity left") {
        auto viewVelOnly = registry.view<Velocity>(ecs::exclude_t<Position>{});
        auto entities = viewVelOnly.get();
        REQUIRE( entities.size() == 1 );
        REQUIRE( entities.front() == e4 );
    }
}

TEST_CASE("Concurrent read access to the same component", "[concurrency][read]") {
    ecs::registry registry;
    constexpr int N = 1000;
    constexpr int R = 4;

    // create N entities with Position{x=1.0}
    for(int i = 0; i < N; ++i)
        registry.create<Position>(Position{1.0f, 0.0f});

    // spawn R reader threads that sum x
    std::vector<float> sums(R, 0.0f);
    std::vector<std::thread> readers;
    for(int t = 0; t < R; ++t) {
        readers.emplace_back([t, &registry, &sums]() {
            float local = 0.0f;
            auto view = registry.view<Position>();
            for(auto e : view.get())
                local += registry.get<Position>(e)->x;
            sums[t] = local;
        });
    }
    for(auto &th : readers) th.join();

    // each reader should see sum == N * 1.0f
    for(int t = 0; t < R; ++t)
        REQUIRE( abs(sums[t] - static_cast<float>(N)) < 0.01 );
}

TEST_CASE("Concurrent entity creation and destruction", "[concurrency][write]") {
    ecs::registry registry;
    constexpr int T = 4;
    constexpr int M = 500;

    // each thread will create M entities of type Health and record them
    std::array<std::vector<ecs::entity>, T> created;
    std::vector<std::thread> creators;
    for(int t = 0; t < T; ++t) {
        creators.emplace_back([t, &registry, &created]() {
            for(int i = 0; i < M; ++i) {
                auto e = registry.create<Health>(Health{100});
                created[t].push_back(e);
            }
        });
    }
    for(auto &th : creators) th.join();

    // total entities == T * M
    auto all_entities = registry.getEntities();
    REQUIRE( static_cast<int>(all_entities.size()) == T * M );

    // now destroy them concurrently in the same thread groups
    std::vector<std::thread> destroyers;
    for(int t = 0; t < T; ++t) {
        destroyers.emplace_back([t, &registry, &created]() {
            for(auto e : created[t]) {
                registry.destroy(e);
            }
        });
    }
    for(auto &th : destroyers) th.join();

    // registry should be empty
    REQUIRE( registry.getEntities().empty() );
}

/** \endcond */