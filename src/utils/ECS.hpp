#pragma once
#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <cassert>
#include <iostream>

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
        ~EntityManager() = default;
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
        ComponentManager() = default;
        ~ComponentManager() = default;

        template <typename Component_t> void registerComponent();
        template <typename Component_t> ComponentID_t getComponentID();
        template <typename Component_t> void addComponent(Entity_t entity, Component_t component);
        template <typename Component_t> void removeComponent(Entity_t entity);
        template <typename Component_t> Component_t &getComponent(Entity_t entity);
        template <typename Component_t> Component_t const &getComponent(Entity_t entity) const;
        void entityDestroyed(Entity_t entity);
    private:
        template <typename Component_t> std::shared_ptr<ComponentArray<Component_t>> getComponentArray();
    };

    class System
    {
    public:
        virtual ~System() = default;
        virtual void update(double deltatime) = 0;
        std::set<Entity_t> m_entities;
    };
     
    class SystemManager
    {
    private:
        std::map<char const *, std::shared_ptr<System>> m_systems{};
        std::map<char const *, Signature_t> m_signatures{};
    public:
        SystemManager() = default;
        ~SystemManager() = default;

        template <typename System_t> std::shared_ptr<System_t> registerSystem();
        template <typename System_t> void setSignature(Signature_t signature);
        void update(double deltatime);
        void addEntity(Entity_t entity);
        void entityDestroyed(Entity_t entity);
        void entitySignatureChanged(Entity_t entity, Signature_t signature);
    };

    // singleton getters
    inline EntityManager &getEntityManager() {
        static EntityManager *manager = new EntityManager{}; // needed to explicitly deallocate opengl entities such as textures before context termination
        return *manager;
    }
    inline ComponentManager &getComponentManager() {
        static ComponentManager *manager = new ComponentManager{};
        return *manager;
    }
    inline SystemManager &getSystemManager() {
        static SystemManager *manager = new SystemManager{};
        return *manager;
    }
    template <typename Component_t> bool entityHasComponent(Entity_t entity);
    template <typename Component_t> Component_t &get(Entity_t entity);
    template <typename... Components_t> ecs::Entity_t makeEntity();
} // namespace ecs


// ============================================================
// ============================================================


inline ecs::EntityManager::EntityManager()
{
    for(Entity_t id = 0; id < MAX_ENTITIES; ++id) {
        m_availableEntityIDs.push(id);
    }
}
inline ecs::Entity_t ecs::EntityManager::createEntity(Signature_t signature)
{
    assert(m_livingEntitiesCount <= MAX_ENTITIES && "too many entities");
    Entity_t entity = m_availableEntityIDs.front();
    m_availableEntityIDs.pop();
    ++m_livingEntitiesCount;
    setSignature(entity, signature);
    return entity;
}
inline void ecs::EntityManager::destroyEntity(Entity_t entity)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);

    m_signatures[entity].reset();
}
inline void ecs::EntityManager::setSignature(Entity_t entity, Signature_t signature)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    m_signatures[entity] = signature;
}
inline ecs::Signature_t const &ecs::EntityManager::getSignature(Entity_t entity) const
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    return m_signatures[entity]; 
}

template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::insert(Entity_t entity, Component_t component)
{
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        std::cout << "WARNING: component added to the same entity more than once!\n";
        return;
    }

    size_t index = m_size++;
    m_entityToIndex[entity] = index;
    m_indexToEntity[index] = entity;
    m_components[index] = component;
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::remove(Entity_t entity)
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "removing non-existing component");
    size_t removedEntityIndex = m_entityToIndex.at(entity);
    size_t lastEntityIndex = m_size-- - 1;
    m_components[removedEntityIndex] = m_components[lastEntityIndex];

    Entity_t lastEntity = m_indexToEntity.at(lastEntityIndex);
    m_entityToIndex.at(lastEntityIndex) = removedEntityIndex;
    m_indexToEntity.at(removedEntityIndex) = lastEntity;
}
template <typename Component_t>
inline Component_t const &ecs::ComponentArray<Component_t>::getComponent(Entity_t entity) const
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline Component_t &ecs::ComponentArray<Component_t>::getComponent(Entity_t entity)
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::onEntityDestroyed(Entity_t entity)
{
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        remove(entity);
    }   
}

template <typename Component_t>
inline void ecs::ComponentManager::registerComponent()
{
    char const *name = typeid(Component_t).name();
    if(m_componentIDs.find(name) != m_componentIDs.end()) {
        return;
    }
    m_componentIDs.insert({name, m_nextID++});
    m_componentArrays.insert({name, std::make_shared<ComponentArray<Component_t>>()});
}
template <typename Component_t>
inline ecs::ComponentID_t ecs::ComponentManager::getComponentID()
{
    char const *name = typeid(Component_t).name();
    assert(m_componentIDs.find(name) != m_componentIDs.end() && "component not registered before use");
    return m_componentIDs.at(name);
}
template <typename Component_t>
inline void ecs::ComponentManager::addComponent(Entity_t entity, Component_t component)
{
    getComponentArray<Component_t>()->insert(entity, component);
}
template <typename Component_t>
inline void ecs::ComponentManager::removeComponent(Entity_t entity)
{
    getComponentArray<Component_t>()->remove(entity);
}
template <typename Component_t>
inline Component_t &ecs::ComponentManager::getComponent(Entity_t entity)
{
    return getComponentArray<Component_t>()->getComponent(entity);
}

template <typename Component_t>
inline Component_t const &ecs::ComponentManager::getComponent(Entity_t entity) const
{
    getComponentArray<Component_t>()->getComponent(entity);
}
template <typename Component_t>
inline std::shared_ptr<ecs::ComponentArray<Component_t>> ecs::ComponentManager::getComponentArray()
{
    char const *name = typeid(Component_t).name();
    assert(m_componentIDs.find(name) != m_componentIDs.end() && "component not registered before use");
    return std::static_pointer_cast<ComponentArray<Component_t>>(m_componentArrays.at(name));
}
inline void ecs::ComponentManager::entityDestroyed(Entity_t entity)
{
    for(auto const &pair : m_componentArrays) {
        pair.second->onEntityDestroyed(entity);
    }
}

template <typename System_t>
inline std::shared_ptr<System_t> ecs::SystemManager::registerSystem()
{
    char const *name = typeid(System_t).name();
    assert(m_systems.find(name) == m_systems.end() && "registering system more than once");

    auto system = std::make_shared<System_t>();
    m_systems.insert({name, system});
    return system;
}
template <typename System_t>
inline void ecs::SystemManager::setSignature(Signature_t signature)
{
    char const *name = typeid(System_t).name();
    assert(m_systems.find(name) != m_systems.end() && "system used before registered");
    m_signatures.insert({name, signature});
}

inline void ecs::SystemManager::update(double deltatime)
{
    for(auto const &pair : m_systems) {
        pair.second->update(deltatime);
    }
}

inline void ecs::SystemManager::addEntity(Entity_t entity)
{
    for(auto const &pair : m_systems) {
        pair.second->m_entities.insert(entity);
    }
}

inline void ecs::SystemManager::entityDestroyed(Entity_t entity)
{
    for(auto const &pair : m_systems) {
        pair.second->m_entities.erase(entity);
    }
}

inline void ecs::SystemManager::entitySignatureChanged(Entity_t entity, Signature_t signature)
{
    for(auto const &pair : m_systems) {
        char const *name = pair.first;
        auto const &system = pair.second;
        auto const &systemSignature = m_signatures.at(name);
        if((signature & systemSignature) == systemSignature) {
            // Entity signature matches system signature - insert into set
            // whut
            system->m_entities.insert(entity);
        }
        else
        {
            // Entity signature does not match system signature - erase from set
            system->m_entities.erase(entity);
        }

    }
}

template <typename Component_t> 
inline bool ecs::entityHasComponent(Entity_t entity) 
{ 
    return getEntityManager().getSignature(entity)[getComponentManager().getComponentID<Component_t>()]; 
}
template <typename Component_t>
inline Component_t &ecs::get(Entity_t entity) 
{
    return getComponentManager().getComponent<Component_t>(entity);
};
template <typename ...Components_t>
inline ecs::Entity_t ecs::makeEntity() 
{
    (getComponentManager().registerComponent<Components_t>(), ...);
    Signature_t signature;
    (signature.set(getComponentManager().getComponentID<Components_t>()), ...);
    Entity_t entity = getEntityManager().createEntity(signature);
    (getComponentManager().addComponent(entity, Components_t{}), ...);
    return entity;
}