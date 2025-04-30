/**
 * \file ECS.hpp
 * \brief My Entity Component System implimentation.
 * 
 * thanks to this post: https://austinmorlan.com/posts/entity_component_system
 * 
 * 
 */

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

namespace ecs
{
    /**
     * \brief Controls the maximum number of entities allowed to exist simultaneously.
     */
    const Entity_t MAX_ENTITIES = 5000;
    /**
     * \brief Controls the maximum number of registered components allowed to exist simultaneously.
     */
    const ComponentID_t MAX_COMPONENTS = 32;

    /**
     * \brief Entity ID.
     */
    using Entity_t = std::uint32_t;
    /**
     * Component ID. Used with Signature_t
     */
    using ComponentID_t = std::uint8_t;
    /**
     * \brief Used to track which components entity has. 
     * As an example, if Transform has type 0, RigidBody has type 1, and Gravity has type 2, an entity that “has” those three components would have a signature of 0b111 (bits 0, 1, and 2 are set).
     */
    using Signature_t = std::bitset<MAX_COMPONENTS>;

    /**
     * \brief Manages entities (create, destroy) and their signatures (set, get).
     * Any entity supplied to the manager must be created by the same manager object.
     */
    class EntityManager
    {
    private:
        std::queue<Entity_t> m_availableEntityIDs;
        std::uint32_t m_livingEntitiesCount = 0;
        std::array<Signature_t, MAX_ENTITIES> m_signatures;
    public:
        EntityManager();
        ~EntityManager() = default;
        /**
         * \brief Creates entity with an optional signature.
         * \returns Unique entity id managed by EntityManager.
         */
        Entity_t createEntity(Signature_t signature = {});
        /**
         * \brief Destroys entity.
         */
        void destroyEntity(Entity_t const &entity);
        /**
         * \brief sets the signature of the entity.
         */
        void setSignature(Entity_t const &entity, Signature_t signature);
        Signature_t const &getSignature(Entity_t const &entity) const;
        Signature_t &getSignature(Entity_t const &entity);
    };

    /**
     * Every instanced ComponentArray derived from that polymorphic class.
     */
    class IComponentArray 
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void onEntityDestroyed(Entity_t const &entity) = 0;
    };

    /**
     * \brief Stores entity Components of a specific type.
     * @tparam Component_t The type of stored components.
     */
    template <typename Component_t>
    class ComponentArray : public IComponentArray
    {
    private:
        std::vector<Component_t> m_components{};
        std::map<Entity_t, size_t> m_entityToIndex{};
        std::map<size_t, Entity_t> m_indexToEntity{};
    public:
        void insert(Entity_t const &entity, Component_t component);
        void remove(Entity_t const &entity);
        Component_t const &getComponent(Entity_t const &entity) const;
        Component_t &getComponent(Entity_t const &entity);
        void onEntityDestroyed(Entity_t const &entity) override;
    };

    /**
     * \brief Manages components and their arrays. All components are destroyed automatically.
     */
    class ComponentManager
    {
    private:
        std::map<char const *, ComponentID_t> m_componentIDs{};
        std::map<char const *, std::shared_ptr<IComponentArray>> m_componentArrays{};
        ComponentID_t m_nextID = 0;
    public:
        ComponentManager() = default;
        ~ComponentManager() = default;

        /**
         * \brief Registers component.
         * This should be called for every component used. Multiple calls for the same Component_t will do nothing.
         * @tparam Component_t The component type.
         */
        template <typename Component_t> void registerComponent();
        /**
         * Get unique component ID used to index the Signature_t bitset.
         */
        template <typename Component_t> ComponentID_t getComponentID();
        template <typename Component_t> void addComponent(Entity_t const &entity, Component_t component);
        template <typename Component_t> void removeComponent(Entity_t const &entity);
        template <typename Component_t> Component_t &getComponent(Entity_t const &entity);
        template <typename Component_t> Component_t const &getComponent(Entity_t const &entity) const;
        void entityDestroyed(Entity_t const &entity);
    private:
        template <typename Component_t> std::shared_ptr<ComponentArray<Component_t>> getComponentArray();
    };

    /**
     * \brief System interface.
     * All systems should derive from that interface.
     */
    class ISystem
    {
    public:
        virtual ~ISystem() = default;
        /**
         * \brief Callback on every system update.
         */
        virtual void update(std::set<Entity_t> const &entities, double deltatime) = 0;
    };

    /**
     * \brief Manages all the systems and the entities supplied to them.
     */
    class SystemManager
    {
    private:
        std::map<char const *, std::shared_ptr<ISystem>> m_systems{};
        std::set<Entity_t> m_entities{};
    public:
        SystemManager() = default;
        ~SystemManager() = default;

        /**
         * This should be called for every system used. Multiple calls for the same System_t will do nothing.
         * @tparam System_t The system type.
         */
        template <typename System_t> std::shared_ptr<System_t> registerSystem();
        template <typename System_t> void removeSystem();
        void update(double deltatime);
        /**
         * adds the entity to the list of entities given to the systems.
         */
        void addEntity(Entity_t const &entity);
        /**
         * removes the entity from the list of entities given to the systems.
         */
        void removeEntity(Entity_t const &entity);
    };

    // singleton getters
    inline EntityManager &getEntityManager() {
        static EntityManager *manager = new EntityManager{}; // needed to explicitly deallocate opengl entities such as textures before context termination.
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
    template <typename Component_t> bool entityHasComponent(Entity_t const &entity);
    template <typename Component_t> Component_t &get(Entity_t const &entity);
    template <typename Component_t> void removeComponent(Entity_t const &entity);
    template <typename Component_t> void addComponent(Entity_t const &entity);
    template <typename... Components_t> ecs::Entity_t makeEntity();
} // namespace ecs


// ===============
// Implimentation
// ===============

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
inline void ecs::EntityManager::destroyEntity(Entity_t const &entity)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);

    m_signatures.at(entity).reset();
}
inline void ecs::EntityManager::setSignature(Entity_t const &entity, Signature_t signature)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    m_signatures[entity] = signature;
}
inline ecs::Signature_t const &ecs::EntityManager::getSignature(Entity_t const &entity) const
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    return m_signatures.at(entity); 
}

inline ecs::Signature_t &ecs::EntityManager::getSignature(Entity_t const &entity)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    return m_signatures.at(entity);
}

template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::insert(Entity_t const &entity, Component_t component)
{
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        std::cout << "WARNING: component added to the same entity more than once!\n";
        return;
    }

    size_t index = m_components.size();
    m_entityToIndex[entity] = index;
    m_indexToEntity[index] = entity;
    m_components.push_back(component);
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::remove(Entity_t const &entity)
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "removing non-existing component");
    size_t removedEntityIndex = m_entityToIndex.at(entity);
    size_t lastEntityIndex = m_components.size() - 1;
    m_components[removedEntityIndex] = m_components[lastEntityIndex];

    Entity_t lastEntity = m_indexToEntity.at(lastEntityIndex);
    m_entityToIndex.at(lastEntityIndex) = removedEntityIndex;
    m_indexToEntity.at(removedEntityIndex) = lastEntity;
    m_components.pop_back();
}
template <typename Component_t>
inline Component_t const &ecs::ComponentArray<Component_t>::getComponent(Entity_t const &entity) const
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline Component_t &ecs::ComponentArray<Component_t>::getComponent(Entity_t const &entity)
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::onEntityDestroyed(Entity_t const &entity)
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
inline void ecs::ComponentManager::addComponent(Entity_t const &entity, Component_t component)
{
    getComponentArray<Component_t>()->insert(entity, component);
}
template <typename Component_t>
inline void ecs::ComponentManager::removeComponent(Entity_t const &entity)
{
    getComponentArray<Component_t>()->remove(entity);
}
template <typename Component_t>
inline Component_t &ecs::ComponentManager::getComponent(Entity_t const &entity)
{
    return getComponentArray<Component_t>()->getComponent(entity);
}

template <typename Component_t>
inline Component_t const &ecs::ComponentManager::getComponent(Entity_t const &entity) const
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
inline void ecs::ComponentManager::entityDestroyed(Entity_t const &entity)
{
    for(auto const &pair : m_componentArrays) {
        pair.second->onEntityDestroyed(entity);
    }
}

template <typename System_t>
inline std::shared_ptr<System_t> ecs::SystemManager::registerSystem()
{
    char const *name = typeid(System_t).name();
    if(m_systems.find(name) == m_systems.end()) return;

    auto system = std::make_shared<System_t>();
    m_systems.insert({name, system});
    return system;
}
template <typename System_t>
inline void ecs::SystemManager::removeSystem()
{
    char const *name = typeid(System_t).name();
    assert(m_systems.find(name) != m_systems.end() && "system not registered before use");
    m_systems.erase(name);
}
inline void ecs::SystemManager::update(double deltatime)
{
    for(auto const &pair : m_systems) {
        pair.second->update(m_entities, deltatime);
    }
}

inline void ecs::SystemManager::addEntity(Entity_t const &entity)
{
    m_entities.insert(entity);
}

inline void ecs::SystemManager::removeEntity(Entity_t const &entity)
{
    m_entities.erase(entity);
}

template <typename Component_t> 
inline bool ecs::entityHasComponent(Entity_t const &entity) 
{ 
    return getEntityManager().getSignature(entity)[getComponentManager().getComponentID<Component_t>()]; 
}
template <typename Component_t>
inline Component_t &ecs::get(Entity_t const &entity) 
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
template <typename Component_t> 
void ecs::removeComponent(Entity_t const &entity) 
{
    getComponentManager().removeComponent<Component_t>(entity);
    getEntityManager().getSignature(entity).set(getComponentManager().getComponentID<Component_t>(), false);
}

template <typename Component_t>
void ecs::addComponent(Entity_t const &entity)
{
    getComponentManager().addComponent<Component_t>(entity);
    getEntityManager().getSignature(entity).set(getComponentManager().getComponentID<Component_t>(), true);
}
