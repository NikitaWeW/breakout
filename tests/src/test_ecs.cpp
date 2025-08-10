#include "catch_amalgamated.hpp"
#include "src/core/ecs.hpp"

struct Position {
    int x = 0, y = 0;
};
struct Velocity {
    int dx = 0, dy = 0;
};
struct Health {
    int value = 100;
};

TEST_CASE("Entity creation, validity and destruction", "[ecs::registry]") {
    ecs::registry reg;

    auto e1 = reg.create();
    auto e2 = reg.create<Position, Velocity>();

    REQUIRE(reg.valid(e1));
    REQUIRE(reg.valid(e2));

    // e1 has no components; e2 has both Position and Velocity
    REQUIRE_FALSE(reg.has<Position>(e1));
    REQUIRE_FALSE(reg.has<Velocity>(e1));

    REQUIRE(reg.has<Position>(e2));
    REQUIRE(reg.has<Velocity>(e2));

    // Destroy e1
    reg.destroy(e1);
    REQUIRE_FALSE(reg.valid(e1));
    REQUIRE_FALSE(reg.has<Position>(e1));
    REQUIRE(reg.valid(e2));

    // getEntities reflects only the remaining valid entity
    auto all = reg.getEntities();
    REQUIRE(all.size() == 1);
    REQUIRE(all.count(e2) == 1);
}

TEST_CASE("Adding, getting and removing components", "[ecs::registry]") {
    ecs::registry reg;
    auto e = reg.create();

    // Initially no components
    REQUIRE_FALSE(reg.has<Position>(e));

    // Add Position by move
    reg.add<Position>(e, Position{10, 20});
    REQUIRE(reg.has<Position>(e));
    auto &pos = reg.get<Position>(e);
    REQUIRE(pos.x == 10);
    REQUIRE(pos.y == 20);

    // Mutate via get()
    reg.get<Position>(e).x = 42;
    REQUIRE(reg.get<Position>(e).x == 42);

    // Add default-constructed Velocity
    reg.add<Velocity>(e);
    REQUIRE(reg.has<Velocity>(e));
    REQUIRE(reg.get<Velocity>(e).dx == 0);
    REQUIRE(reg.get<Velocity>(e).dy == 0);

    // Remove Position
    reg.remove<Position>(e);
    REQUIRE_FALSE(reg.has<Position>(e));
}

TEST_CASE("View with include and exclude filters", "[ecs::registry]") {
    ecs::registry reg;

    // Create various entities
    auto eA  = reg.create<Position>();
    auto eAB = reg.create<Position, Velocity>();
    auto eB  = reg.create<Velocity>();
    auto e0  = reg.create<>();

    // View entities that have Position
    auto withPos = reg.view<Position>();
    std::vector<ecs::entity> expectedPos = { eA, eAB };
    REQUIRE(withPos.size() == expectedPos.size());
    for (auto ent : expectedPos) {
        REQUIRE(std::find(withPos.begin(), withPos.end(), ent) != withPos.end());
    }

    // View entities that have Position and Velocity
    auto withBoth = reg.view<Position, Velocity>();
    REQUIRE(withBoth.size() == 1);
    REQUIRE(withBoth[0] == eAB);

    // View entities that have Position but exclude Velocity
    auto onlyPos = reg.view<Position>(ecs::registry::exclude_t<Velocity>{});
    REQUIRE(onlyPos.size() == 1);
    REQUIRE(onlyPos[0] == eA);

    // View entities that have neither Position nor Velocity
    auto none = reg.view<Health>(ecs::registry::exclude_t<Position, Velocity>{});
    REQUIRE(none.empty());
}

TEST_CASE("Component ID consistency", "[ecs::registry]") {
    ecs::registry reg;
    auto idP1 = reg.getComponentID<Position>();
    auto idV  = reg.getComponentID<Velocity>();
    auto idP2 = reg.getComponentID<Position>();

    REQUIRE(idP1 == idP2);
    REQUIRE(idP1 != idV);
}

TEST_CASE("Signature bit-count matches number of components", "[ecs::registry]") {
    ecs::registry reg;
    auto e = reg.create<Position, Velocity, Health>();

    auto sig = reg.getSignature(e);
    // Signature_t is a bitset: count() yields number of set bits
    REQUIRE(sig.count() == 3);
}

TEST_CASE("Systems can be added, updated, and removed", "[ecs::registry]") {
    ecs::registry reg;
    static int globalCount = 0;

    struct CounterSystem : ecs::ISystem {
        void update(ecs::registry&) override {
            ++globalCount;
        }
    };

    // Add system and call update
    reg.addSystem<CounterSystem>();
    REQUIRE(globalCount == 0);

    reg.update();
    REQUIRE(globalCount == 1);

    reg.update();
    REQUIRE(globalCount == 2);

    // Remove system and ensure update no longer increments
    reg.removeSystem<CounterSystem>();
    reg.update();
    REQUIRE(globalCount == 2);
}

struct Dummy {
    int value = 0;
};

TEST_CASE("ComponentArray basic operations", "[ecs::ComponentArray]") {
    ecs::ComponentArray<int> arr;
    ecs::entity e1{1}, e2{2};

    // Inserting and retrieving values
    REQUIRE_NOTHROW(arr.insert(e1, 10));
    REQUIRE(arr.getComponent(e1) == 10);

    // Mutating via non-const get
    arr.getComponent(e1) = 15;
    REQUIRE(arr.getComponent(e1) == 15);

    // Insert a second entity
    REQUIRE_NOTHROW(arr.insert(e2, 20));
    REQUIRE(arr.getComponent(e2) == 20);

    // Remove the first entity
    REQUIRE_NOTHROW(arr.remove(e1));
    REQUIRE_THROWS_AS(arr.getComponent(e1), std::out_of_range);

    // onEntityDestroyed should also remove the component
    REQUIRE_NOTHROW(arr.insert(e1, 30));
    REQUIRE(arr.getComponent(e1) == 30);
    REQUIRE_NOTHROW(arr.onEntityDestroyed(e1));
    REQUIRE_THROWS_AS(arr.getComponent(e1), std::out_of_range);

    // Removing a non‐existent entity should be a no-op
    REQUIRE_NOTHROW(arr.remove(9999));
    REQUIRE_NOTHROW(arr.onEntityDestroyed(9999));
}

TEST_CASE("ComponentManager full workflow", "[ecs::ComponentManager]") {
    ecs::ComponentManager cm;
    ecs::entity e{42};

    // Register component types and verify IDs are stable and unique
    cm.registerComponent<Dummy>();
    auto dummyID1 = cm.getComponentID<Dummy>();
    cm.registerComponent<Dummy>();               // second registration is a no-op
    auto dummyID2 = cm.getComponentID<Dummy>();
    REQUIRE(dummyID1 == dummyID2);

    cm.registerComponent<int>();
    auto intID = cm.getComponentID<int>();
    REQUIRE(intID != dummyID1);

    // Add components and retrieve them
    REQUIRE_NOTHROW(cm.addComponent<Dummy>(e, Dummy{5}));
    REQUIRE(cm.getComponent<Dummy>(e).value == 5);

    REQUIRE_NOTHROW(cm.addComponent<int>(e, 123));
    REQUIRE(cm.getComponent<int>(e) == 123);

    // Remove one component
    REQUIRE_NOTHROW(cm.removeComponent<Dummy>(e));
    REQUIRE_THROWS_AS(cm.getComponent<Dummy>(e), std::out_of_range);

    // entityDestroyed should clear all remaining components
    REQUIRE_NOTHROW(cm.entityDestroyed(e));
    REQUIRE_THROWS_AS(cm.getComponent<int>(e), std::out_of_range);

    // Re-add after destruction to ensure cleanup was complete
    REQUIRE_NOTHROW(cm.addComponent<int>(e, 999));
    REQUIRE(cm.getComponent<int>(e) == 999);
}

TEST_CASE("Entity creation, uniqueness, and validity", "[EntityManager]") {
    ecs::EntityManager em;

    // Create two entities
    auto e1 = em.createEntity();
    auto e2 = em.createEntity();

    REQUIRE(e1 != e2);
    REQUIRE(em.valid(e1));
    REQUIRE(em.valid(e2));

    // getEntities should contain both
    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == 2);
        REQUIRE(entities.count(e1) == 1);
        REQUIRE(entities.count(e2) == 1);
    }

    // A completely out-of-range ID is invalid
    ecs::entity bogus{ ecs::MAX_ENTITIES + 100 };
    REQUIRE_FALSE(em.valid(bogus));
}

TEST_CASE("Destroying entities and reusing IDs", "[EntityManager]") {
    ecs::EntityManager em;

    auto e1 = em.createEntity();
    auto e2 = em.createEntity();

    // Destroy e1
    em.destroyEntity(e1);
    REQUIRE_FALSE(em.valid(e1));
    REQUIRE(em.valid(e2));

    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.count(e1) == 0);
        REQUIRE(entities.count(e2) == 1);
    }

    // The next createEntity should reuse the freed ID (e1)
    auto e3 = em.createEntity();
    REQUIRE(e3 == e1);
    REQUIRE(em.valid(e3));

    // Now two alive: e2 and e3
    {
        auto const &entities = em.getEntities();
        REQUIRE(entities.size() == 2);
        REQUIRE(entities.count(e2) == 1);
        REQUIRE(entities.count(e3) == 1);
    }
}

TEST_CASE("Signature get/set and consistency", "[EntityManager]") {
    ecs::EntityManager em;
    auto e = em.createEntity();

    // Initially no bits set
    auto &sigRef = em.getSignature(e);
    REQUIRE(sigRef.none());

    // Mutate signature via non-const reference
    sigRef.set(3);
    REQUIRE(sigRef.test(3));
    REQUIRE(sigRef.count() == 1);

    // Replace signature wholesale
    ecs::Signature_t newSig;
    newSig.set(5).set(7);
    em.setSignature(e, newSig);

    auto const &sigConst = em.getSignature(e);
    REQUIRE(sigConst.test(5));
    REQUIRE(sigConst.test(7));
    REQUIRE_FALSE(sigConst.test(3));
    REQUIRE(sigConst.count() == 2);

    // Const overload works the same
    auto const &cem = em;
    auto const &sigConst2 = cem.getSignature(e);
    REQUIRE(sigConst2.test(5));
    REQUIRE(sigConst2.test(7));
}

TEST_CASE("Creating maximum entities", "[EntityManager]") {
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
