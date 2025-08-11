#include "catch_amalgamated.hpp"
#include "src/core/ecs.hpp"
#include <atomic>
#include <future>
#include <random>

struct Position { int x = 0,  y = 0;  };
struct Velocity { int dx = 0, dy = 0; };
struct Health   { int value = 100;    };
struct Dummy    { int value = 0;      };
struct Position { float x, y;         };
struct Velocity { float dx, dy;       };
struct Tag : std::string {};

TEST_CASE("Entity creation, validity and destruction", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;

    auto e1 = reg.create();
    auto e2 = reg.create<Position, Velocity>();

    REQUIRE(reg.valid(e1));
    REQUIRE(reg.valid(e2));

    REQUIRE_FALSE(reg.has<Position>(e1));
    REQUIRE_FALSE(reg.has<Velocity>(e1));

    REQUIRE(reg.has<Position>(e2));
    REQUIRE(reg.has<Velocity>(e2));

    reg.destroy(e1);
    REQUIRE_FALSE(reg.valid(e1));
    REQUIRE_FALSE(reg.has<Position>(e1));
    REQUIRE(reg.valid(e2));

    auto all = reg.getEntities();
    REQUIRE(all.size() == 1);
    REQUIRE(all.count(e2) == 1);
}
TEST_CASE("Adding, getting and removing components", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    auto e = reg.create();

    REQUIRE_FALSE(reg.has<Position>(e));

    reg.add<Position>(e, Position{10, 20});
    REQUIRE(reg.has<Position>(e));
    auto &pos = reg.get<Position>(e);
    REQUIRE(pos.x == 10);
    REQUIRE(pos.y == 20);

    reg.get<Position>(e).x = 42;
    REQUIRE(reg.get<Position>(e).x == 42);

    reg.add<Velocity>(e);
    REQUIRE(reg.has<Velocity>(e));
    REQUIRE(reg.get<Velocity>(e).dx == 0);
    REQUIRE(reg.get<Velocity>(e).dy == 0);

    reg.remove<Position>(e);
    REQUIRE_FALSE(reg.has<Position>(e));
}
TEST_CASE("View with include and exclude filters", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;

    auto eA  = reg.create<Position>();
    auto eAB = reg.create<Position, Velocity>();
    auto eB  = reg.create<Velocity>();
    auto e0  = reg.create<>();

    auto withPos = reg.view<Position>();
    std::vector<ecs::entity> expectedPos = { eA, eAB };
    REQUIRE(withPos.size() == expectedPos.size());
    for (auto ent : expectedPos) {
        REQUIRE(std::find(withPos.begin(), withPos.end(), ent) != withPos.end());
    }

    auto withBoth = reg.view<Position, Velocity>();
    REQUIRE(withBoth.size() == 1);
    REQUIRE(withBoth[0] == eAB);

    for(auto e : withBoth)
    {
        REQUIRE(reg.has<Position>(e));
        REQUIRE(reg.has<Velocity>(e));
        REQUIRE(reg.getSignature(e).count() == 2);
    }

    auto onlyPos = reg.view<Position>(ecs::registry::exclude_t<Velocity>{});
    REQUIRE(onlyPos.size() == 1);
    REQUIRE(onlyPos[0] == eA);

    for(auto e : onlyPos)
    {
        REQUIRE(reg.has<Position>(e));
        REQUIRE_FALSE(reg.has<Velocity>(e));
        REQUIRE(reg.getSignature(e).count() == 1);
    }

    auto none = reg.view<Health>(ecs::registry::exclude_t<Position, Velocity>{});
    REQUIRE(none.empty());
}
TEST_CASE("Component ID consistency", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    auto idP1 = reg.getComponentID<Position>();
    auto idV  = reg.getComponentID<Velocity>();
    auto idP2 = reg.getComponentID<Position>();

    REQUIRE(idP1 == idP2);
    REQUIRE(idP1 != idV);
}
TEST_CASE("Signature bit-count matches number of components", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    auto e = reg.create<Position, Velocity, Health>();

    auto sig = reg.getSignature(e);
    REQUIRE(sig.count() == 3);
}
TEST_CASE("Systems can be added, updated, and removed", "[ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    static int globalCount = 0;

    struct CounterSystem : ecs::ISystem {
        void update(ecs::registry&) override {
            ++globalCount;
        }
    };

    reg.addSystem<CounterSystem>();
    REQUIRE(globalCount == 0);

    reg.update();
    REQUIRE(globalCount == 1);

    reg.update();
    REQUIRE(globalCount == 2);

    reg.removeSystem<CounterSystem>();
    reg.update();
    REQUIRE(globalCount == 2);
}
TEST_CASE("ComponentArray basic operations", "[ecs::ComponentArray]") 
{
    ECS_PROFILE();

    ecs::ComponentArray<int> arr;
    ecs::entity e1{1}, e2{2};

    REQUIRE_NOTHROW(arr.insert(e1, 10));
    REQUIRE(arr.getComponent(e1) == 10);

    arr.getComponent(e1) = 15;
    REQUIRE(arr.getComponent(e1) == 15);

    REQUIRE_NOTHROW(arr.insert(e2, 20));
    REQUIRE(arr.getComponent(e2) == 20);

    REQUIRE_NOTHROW(arr.remove(e1));
    REQUIRE_THROWS_AS(arr.getComponent(e1), std::out_of_range);

    REQUIRE_NOTHROW(arr.insert(e1, 30));
    REQUIRE(arr.getComponent(e1) == 30);
    REQUIRE_NOTHROW(arr.onEntityDestroyed(e1));
    REQUIRE_THROWS_AS(arr.getComponent(e1), std::out_of_range);

    REQUIRE_NOTHROW(arr.remove(9999));
    REQUIRE_NOTHROW(arr.onEntityDestroyed(9999));
}
TEST_CASE("ComponentManager full workflow", "[ecs::ComponentManager]") 
{
    ECS_PROFILE();

    ecs::ComponentManager cm;
    ecs::entity e{42};

    cm.registerComponent<Dummy>();
    auto dummyID1 = cm.getComponentID<Dummy>();
    auto dummyID2 = cm.getComponentID<Dummy>();
    REQUIRE(dummyID1 == dummyID2);

    cm.registerComponent<int>();
    auto intID = cm.getComponentID<int>();
    REQUIRE(intID != dummyID1);

    REQUIRE_NOTHROW(cm.addComponent<Dummy>(e, Dummy{5}));
    REQUIRE(cm.getComponent<Dummy>(e).value == 5);

    REQUIRE_NOTHROW(cm.addComponent<int>(e, 123));
    REQUIRE(cm.getComponent<int>(e) == 123);

    REQUIRE_NOTHROW(cm.removeComponent<Dummy>(e));
    REQUIRE_THROWS_AS(cm.getComponent<Dummy>(e), std::out_of_range);

    REQUIRE_NOTHROW(cm.entityDestroyed(e));
    REQUIRE_THROWS_AS(cm.getComponent<int>(e), std::out_of_range);

    REQUIRE_NOTHROW(cm.addComponent<int>(e, 999));
    REQUIRE(cm.getComponent<int>(e) == 999);
}
TEST_CASE("Entity creation, uniqueness, and validity", "[ecs::EntityManager]") 
{
    ECS_PROFILE();

    ecs::EntityManager em;

    auto e1 = em.createEntity();
    auto e2 = em.createEntity();

    REQUIRE(e1 != e2);
    REQUIRE(em.valid(e1));
    REQUIRE(em.valid(e2));

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == 2);
        REQUIRE(entities.count(e1) == 1);
        REQUIRE(entities.count(e2) == 1);
    }

    ecs::entity bogus{ ecs::MAX_ENTITIES + 100 };
    REQUIRE_FALSE(em.valid(bogus));
}
TEST_CASE("Destroying entities and reusing IDs", "[ecs::EntityManager]") 
{
    ECS_PROFILE();

    ecs::EntityManager em;

    auto e1 = em.createEntity();
    auto e2 = em.createEntity();

    em.destroyEntity(e1);
    REQUIRE_FALSE(em.valid(e1));
    REQUIRE(em.valid(e2));

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.count(e1) == 0);
        REQUIRE(entities.count(e2) == 1);
    }

    auto e3 = em.createEntity();
    REQUIRE(e3 == e1);
    REQUIRE(em.valid(e3));

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == 2);
        REQUIRE(entities.count(e2) == 1);
        REQUIRE(entities.count(e3) == 1);
    }
}
TEST_CASE("Signature get/set and consistency", "[ecs::EntityManager]") 
{
    ECS_PROFILE();

    ecs::EntityManager em;
    auto e = em.createEntity();

    auto &sigRef = em.getSignature(e);
    REQUIRE(sigRef.none());

    sigRef.set(3);
    REQUIRE(sigRef.test(3));
    REQUIRE(sigRef.count() == 1);

    ecs::Signature_t newSig;
    newSig.set(5).set(7);
    em.setSignature(e, newSig);

    auto const &sigConst = em.getSignature(e);
    REQUIRE(sigConst.test(5));
    REQUIRE(sigConst.test(7));
    REQUIRE_FALSE(sigConst.test(3));
    REQUIRE(sigConst.count() == 2);

    auto const &cem = em;
    auto const &sigConst2 = cem.getSignature(e);
    REQUIRE(sigConst2.test(5));
    REQUIRE(sigConst2.test(7));
}
TEST_CASE("Creating maximum entities", "[ecs::EntityManager]") 
{
    ECS_PROFILE();

    ecs::EntityManager em;
    std::vector<ecs::entity> created;
    created.reserve(ecs::MAX_ENTITIES);

    for (std::size_t i = 0; i < ecs::MAX_ENTITIES; ++i) {
        created.push_back(em.createEntity());
    }

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == ecs::MAX_ENTITIES);
    }

    for (std::uint32_t i = 0; i < ecs::MAX_ENTITIES; ++i) {
        REQUIRE(em.valid({i}));
    }

    for (std::size_t i = 0; i < ecs::MAX_ENTITIES; i += 2) {
        em.destroyEntity(created[i]);
    }

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == ecs::MAX_ENTITIES / 2);
    }

    for (std::size_t i = 0; i < ecs::MAX_ENTITIES / 2; ++i) {
        auto e = em.createEntity();
        REQUIRE((e % 2) == 0);
        REQUIRE(em.valid(e));
    }
}
TEST_CASE("Concurrent Entity Creation", "[concurrency][ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;
    constexpr int threads = 8;
    constexpr int perThread = 1000;

    std::mutex mtx;
    std::vector<ecs::entity> allEntities;
    allEntities.reserve(threads * perThread);

    // Launch multiple async tasks to create entities
    std::vector<std::future<void>> futures;
    for(int t = 0; t < threads; ++t) {
        futures.push_back(std::async(std::launch::async, [&](){
            std::vector<ecs::entity> local;
            local.reserve(perThread);
            for(int i = 0; i < perThread; ++i) {
                local.push_back(reg.create<>());
            }
            std::lock_guard<std::mutex> lock(mtx);
            allEntities.insert(allEntities.end(), local.begin(), local.end());
        }));
    }

    for(auto &f : futures) {
        f.get();
    }

    REQUIRE(reg.getEntities().size() == static_cast<size_t>(threads * perThread));
    REQUIRE(allEntities.size() == static_cast<size_t>(threads * perThread));
}
TEST_CASE("Concurrent Component Addition", "[concurrency][ecs::registry]") 
{
    ECS_PROFILE();


    ecs::registry reg;

    constexpr int count = 2000;
    for(int i = 0; i < count; ++i) {
        reg.create<>();
    }

    auto entities = reg.getEntities();
    std::vector<ecs::entity> list(entities.begin(), entities.end());
    std::atomic_uint numThrows = 0;

    auto worker = [&](int start, int end) {
        for(int i = start; i < end; ++i) {
            try {
                reg.add<Position>(list[i], Position{float(i), float(i)*2});
            } catch(std::invalid_argument)
            {
                ++numThrows;
            }
        }
    };

    auto mid = count / 2;
    auto f1  = std::async(std::launch::async, worker, 0, mid);
    auto f2  = std::async(std::launch::async, worker, mid, count);
    auto f3  = std::async(std::launch::async, worker, 0, count); // should be count throws

    f1.get(); f2.get(); f3.get();

    REQUIRE(numThrows.load() == count);

    for(auto &e : list) {
        REQUIRE(reg.has<Position>(e));
        auto const &p = reg.get<Position>(e);
        REQUIRE(p.x * 2 == p.y);
    }
}
TEST_CASE("Concurrent Component Removal", "[concurrency][ecs::registry]") 
{
    ECS_PROFILE();

    ecs::registry reg;

    constexpr int count = 2000;
    for(int i = 0; i < count; ++i) {
        reg.create<>();
    }

    auto entities = reg.getEntities();
    std::vector<ecs::entity> list(entities.begin(), entities.end());
    std::atomic_uint numThrows = 0;

    auto worker = [&](int start, int end) {
        for(int i = start; i < end; ++i) {
            try {
                reg.remove<Position>(list[i]);
            } catch(std::invalid_argument)
            {
                ++numThrows;
            }
        }
    };

    auto mid = count / 2;
    auto f1  = std::async(std::launch::async, worker, 0, mid);
    auto f2  = std::async(std::launch::async, worker, mid, count);
    auto f3  = std::async(std::launch::async, worker, 0, count); // should be count throws

    f1.get(); f2.get(); f3.get();

    REQUIRE(numThrows.load() == count);

    for(auto &e : list) {
        REQUIRE(reg.has<Position>(e));
        auto const &p = reg.get<Position>(e);
        REQUIRE(p.x * 2 == p.y);
    }

    // no iterator invalidation
    for(auto &e : list)
    {
        reg.destroy(e);
    }
}
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
                auto &pos = reg.get<Position>(e);
                pos.x += std::sin(float(i)) * 0.1f;
                pos.y -= std::cos(float(i)) * 0.05f;
            }
            if(reg.has<Velocity>(e)) {
                auto &vel = reg.get<Velocity>(e);
                vel.dx *= (1.0f + 0.01f * (i % 10));
                vel.dy *= (1.0f - 0.01f * (i % 15));
            }

            if(i % 11 == 0) {
                auto const &sig = reg.getSignature(e);
                size_t bitcount = sig.count();
                (void)bitcount; // no-op
            }
            if(i % 13 == 0) {
                auto viewPosVel = reg.view<Position, Velocity>(ecs::registry::exclude_t<Health>{});
                if(!viewPosVel.empty()) {
                    auto some = viewPosVel[rand() % viewPosVel.size()];
                    auto &v = reg.get<Velocity>(some);
                    v.dx = -v.dx;
                    v.dy = -v.dy;
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
