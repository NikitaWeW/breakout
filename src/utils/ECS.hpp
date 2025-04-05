#pragma once
#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <map>
#include <memory>
#include <set>

// thanks to https://austinmorlan.com/posts/entity_component_system
namespace ecs
{
    // entity ID
    using Entity_t = std::uint32_t;
    // component ID. Used with EntitySignature_t
    using ComponentID_t = std::uint8_t;

    const Entity_t MAX_ENTITIES = 5000;
    const ComponentID_t MAX_COMPONENTS = 32;

    // Used to track which components entity has. As an example, if Transform has type 0, RigidBody has type 1, and Gravity has type 2, an entity that “has” those three components would have a signature of 0b111 (bits 0, 1, and 2 are set).
    using Signature_t = std::bitset<MAX_COMPONENTS>;

    class EntityManager
    {
    private:
        std::queue<Entity_t> m_availableEntityIDs;
        std::uint32_t m_livingEntitiesCount = 0;
        std::array<Signature_t, MAX_ENTITIES> m_signatures;
    public:
        EntityManager();
        Entity_t createEntity(Signature_t signature = {});
        void destroyEntity(Entity_t entity);
        void setSignature(Entity_t entity, Signature_t signature);
        Signature_t const &getSignature(Entity_t entity) const;
    };

    class IComponentArray 
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void onEntityDestroyed(Entity_t entity) = 0;
    };

    template <typename Component_t>
    class ComponentArray : public IComponentArray
    {
    private:
        std::array<Component_t, MAX_COMPONENTS> m_components{};
        std::map<Entity_t, size_t> m_entityToIndex{};
        std::map<size_t, Entity_t> m_indexToEntity{};
        size_t m_size = 0;
    public:
        void insert(Entity_t entity, Component_t component);
        void remove(Entity_t entity);
        Component_t &getComponent(Entity_t entity);
        Component_t const &getComponent(Entity_t entity) const;
        void onEntityDestroyed(Entity_t entity) override;
    };

    class ComponentManager
    {
    private:
        std::map<char const *, ComponentID_t> m_componentIDs{};
        std::map<char const *, std::shared_ptr<IComponentArray>> m_componentArrays{};
        ComponentID_t m_nextID = 0;
    public:
        template <typename Component_t> void registerComponent();
        template <typename Component_t> ComponentID_t getComponentID();
        template <typename Component_t> void addComponent(Entity_t entity, Component_t component);
        template <typename Component_t> void removeComponent(Entity_t entity);
        template <typename Component_t> Component_t &getComponent(Entity_t entity);
        template <typename Component_t> Component_t const &getComponent(Entity_t entity) const;
        void entityDestroyed(Entity_t entity);
    private:
        template <typename Component_t> std::shared_ptr<ecs::ComponentArray<Component_t>> getComponentArray();
    };

    class System
    {
    public:
        virtual ~System() = default;
        std::set<Entity_t> m_entities;
    };
     
    class SystemManager
    {
    private:
        std::map<char const *, std::shared_ptr<System>> m_systems{};
        std::map<char const *, Signature_t> m_signatures{};
    public:
        template <typename System_t> std::shared_ptr<System_t> registerSystem();
        template <typename System_t> void setSignature(Signature_t signature);
        void entityDestroyed(Entity_t entity);
        void entitySignatureChanged(Entity_t entity, Signature_t signature);
    };

    // singleton getters
    inline EntityManager &getEntityManager() {
        static EntityManager manager{};
        return manager;
    }
    inline ComponentManager &getComponentManager() {
        static ComponentManager manager{};
        return manager;
    }
    inline SystemManager &getSystemManager() {
        static SystemManager manager{};
        return manager;
    }
} // namespace ecs
