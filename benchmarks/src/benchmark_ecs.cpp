// Build in release mode

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <entt/entt.hpp>
#include "core/ecs.hpp"

/*! \cond Doxygen_Suppress */

struct Position { float x, y; };
struct Velocity { float x, y; };
struct Health   { int   hp; };

static constexpr std::size_t EntityCount = 10000;

TEST_CASE("Create entities", "[benchmark]") {
    BENCHMARK("entt: create() " + std::to_string(EntityCount) + " entities") {
        entt::registry reg;
        for (std::size_t i = 0; i < EntityCount; ++i) {
            auto dummy = reg.create();
            (void)dummy;
        }
        return EntityCount;
    };

    BENCHMARK("custom ecs: create() " + std::to_string(EntityCount) + " entities") {
        ecs::registry reg;
        for (std::size_t i = 0; i < EntityCount; ++i) {
            reg.create();
        }
        return reg.getEntities().size();
    };
}

TEST_CASE("Add three components", "[benchmark]") {
    BENCHMARK("entt: assign<Position, Velocity, Health> to " + std::to_string(EntityCount) + " entities") {
        entt::registry reg;
        for (std::size_t i = 0; i < EntityCount; ++i) {
            auto e = reg.create();
            reg.emplace<Position>(e, float(i), float(i)*2.0f);
            reg.emplace<Velocity>(e, float(i)*0.1f, float(i)*0.2f);
            reg.emplace<Health>(e, 100);
        }
        return EntityCount;
    };

    BENCHMARK("custom ecs: add<Position, Velocity, Health> to " + std::to_string(EntityCount) + " entities") {
        ecs::registry reg;
        for (std::size_t i = 0; i < EntityCount; ++i) {
            auto e = reg.create();
            reg.add<Position>(e, Position{float(i), float(i)*2.0f});
            reg.add<Velocity>(e, Velocity{float(i)*0.1f, float(i)*0.2f});
            reg.add<Health>(e,   Health{100});
        }
        return reg.getEntities().size();
    };
}

TEST_CASE("Iterate view of entities with Position & Velocity", "[benchmark]") {
    // prepare a registry pre-populated with all components
    entt::registry entt_reg;
    ecs::registry  custom_reg;

    for (std::size_t i = 0; i < EntityCount; ++i) {
        auto e1 = entt_reg.create();
        entt_reg.emplace<Position>(e1, float(i), float(i)+1.0f);
        entt_reg.emplace<Velocity>(e1, float(i)*0.5f, float(i)*0.5f);

        auto e2 = custom_reg.create();
        custom_reg.add<Position>(e2, Position{float(i), float(i)+1.0f});
        custom_reg.add<Velocity>(e2, Velocity{float(i)*0.5f, float(i)*0.5f});
    }

    BENCHMARK("entt: view(Position, Velocity) iteration") {
        std::size_t count = 0;
        auto view = entt_reg.view<Position, Velocity>();
        for (auto [entity, pos, vel]: view.each()) {
            // simple compute
            pos.x += vel.x;
            pos.y += vel.y;
            ++count;
        }
        return count;
    };

    BENCHMARK("custom ecs: view<Position, Velocity> iteration") {
        std::size_t count = 0;
        auto entities = custom_reg.view<Position, Velocity>();
        for (auto entity: entities) {
            auto &pos = custom_reg.get<Position>(entity);
            auto &vel = custom_reg.get<Velocity>(entity);
            pos.x += vel.x;
            pos.y += vel.y;
            ++count;
        }
        return count;
    };
}

/*! \endcond */