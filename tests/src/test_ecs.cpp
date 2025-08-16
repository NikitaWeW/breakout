#include "catch2/catch_test_macros.hpp"
#include "core/ecs.hpp"
#include <atomic>
#include <future>
#include <random>

struct Position { int x = 0,  y = 0;  };
struct Velocity { int dx = 0, dy = 0; };
struct Health   { int value = 100;    };
struct Tag : std::string {};

TEST_CASE("Mixed Concurrent ECS Operations", "[concurrency][ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    constexpr int opsPerThread = 500;

    auto worker = [&](){
        std::mt19937_64 rng{std::random_device{}()};
        std::uniform_int_distribution<int> coin(0, 3);

        for(int i = 0; i < opsPerThread; ++i) {
            ecs::entity e;
            switch(coin(rng)) {
                case 0: e = reg.create<Position>();                 break;
                case 1: e = reg.create<Position, Velocity>();       break;
                case 2: e = reg.create<Velocity, Health>();         break;
                default: e = reg.create<Position, Velocity, Health>(); break;
            }

            if(i % 5 == 0) {
                if(!reg.has<Tag>(e)) {
                    reg.add<Tag>(e, Tag{"worker_tag"});
                }
            } else if(i % 7 == 0) {
                if(reg.has<Health>(e)) {
                    reg.remove<Health>(e);
                }
            }

            if(reg.has<Position>(e)) {
                auto pos = reg.lock<Position>(e);
                pos->x += std::sin(float(i)) * 0.1f;
                pos->y -= std::cos(float(i)) * 0.05f;
            }
            if(reg.has<Velocity>(e)) {
                auto vel = reg.lock<Velocity>(e);
                vel->dx *= (1.0f + 0.01f * (i % 10));
                vel->dy *= (1.0f - 0.01f * (i % 15));
            }

            if(i % 11 == 0) {
                auto sig = reg.getSignature(e);
                size_t bitcount = sig.count();
                (void)bitcount; // no-op
            }
            if(i % 13 == 0) {
                auto viewPosVel = reg.view<Position, Velocity>(ecs::registry::exclude_t<Health>{});
                if(!viewPosVel.empty()) {
                    auto some = viewPosVel[rand() % viewPosVel.size()];
                    auto v = reg.lock<Velocity>(some);
                    v->dx = -v->dx;
                    v->dy = -v->dy;
                }

                for(auto e : viewPosVel)
                {
                    REQUIRE(reg.has<Position>(e));
                    REQUIRE(reg.has<Velocity>(e));
                    REQUIRE_FALSE(reg.has<Health>(e));
                }
            }
            if(i % 17 == 0) {
                reg.update();
            }

            if(reg.has<Velocity>(e)) {
                reg.remove<Velocity>(e);
            } else {
                REQUIRE_THROWS_AS(reg.remove<Velocity>(e), std::out_of_range);
            }

            if((i % 3 == 0) && coin(rng) == 0) {
                reg.destroy(e);
            }
        }
    };


    std::future<void> a = std::async(std::launch::async, worker);
    std::future<void> b = std::async(std::launch::async, worker);
    std::future<void> c = std::async(std::launch::async, worker);

    a.get(); b.get(); c.get();

    for(auto e : reg.getEntities()) {
        REQUIRE(reg.valid(e));
        REQUIRE_FALSE(reg.has<Velocity>(e));
    }
}
