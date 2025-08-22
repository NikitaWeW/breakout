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

/** \endcond */