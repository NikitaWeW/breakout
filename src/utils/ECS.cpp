#include "ECS.hpp"
#include <cassert>

ecs::EntityManager::EntityManager()
{
    for(Entity_t id = 0; id < MAX_ENTITIES; ++id) {
        m_availableEntityIDs.push(id);
    }
}
ecs::Entity_t ecs::EntityManager::createEntity(Signature_t signature)
{
    assert(m_livingEntitiesCount <= MAX_ENTITIES && "too many entities");
    Entity_t entity = m_availableEntityIDs.front();
    m_availableEntityIDs.pop();
    ++m_livingEntitiesCount;
    setSignature(entity, signature);
    return entity;
}
void ecs::EntityManager::destroyEntity(Entity_t entity)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);

    m_signatures[entity].reset();
}
void ecs::EntityManager::setSignature(Entity_t entity, Signature_t signature)
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    m_signatures[entity] = signature;
}
ecs::Signature_t const &ecs::EntityManager::getSignature(Entity_t entity) const
{
    assert(entity <= MAX_ENTITIES && "entity out of range");
    return m_signatures[entity]; 
}

template <typename Component_t>
void ecs::ComponentArray<Component_t>::insert(Entity_t entity, Component_t component)
{
    assert(m_entityToIndex.find(entity) == m_entityToIndex.end() && "component added to the same entity more than once");

    size_t index = m_size++;
    m_entityToIndex[entity] = index;
    m_indexToEntity[index] = entity;
    m_components[index] = component;
}
template <typename Component_t>
void ecs::ComponentArray<Component_t>::remove(Entity_t entity)
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
Component_t const &ecs::ComponentArray<Component_t>::getComponent(Entity_t entity) const
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
Component_t &ecs::ComponentArray<Component_t>::getComponent(Entity_t entity)
{
    assert(m_entityToIndex.find(entity) != m_entityToIndex.end() && "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
void ecs::ComponentArray<Component_t>::onEntityDestroyed(Entity_t entity)
{
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        remove(entity);
    }   
}

template <typename Component_t>
void ecs::ComponentManager::registerComponent()
{
    char const *name = typeid(Component_t).name();
    assert(m_componentIDs.find(name) == m_componentIDs.end() && "registering component type more than once");
    m_componentIDs.insert({name, m_nextID++});
    m_componentArrays.insert({name, std::make_shared<ComponentArray<Component_t>()});
}
template <typename Component_t>
ecs::ComponentID_t ecs::ComponentManager::getComponentID()
{
    char const *name = typeid(Component_t).name();
    assert(m_componentIDs.find(name) != m_componentIDs.end() && "component not registered before use");
    return m_componentIDs.at(name);
}
template <typename Component_t>
void ecs::ComponentManager::addComponent(Entity_t entity, Component_t component)
{
    getComponentArray<Component_t>()->insert(entity, component);
}
template <typename Component_t>
void ecs::ComponentManager::removeComponent(Entity_t entity)
{
    getComponentArray<Component_t>()->remove(entity);
}
template <typename Component_t>
Component_t &ecs::ComponentManager::getComponent(Entity_t entity)
{
    getComponentArray<Component_t>()->getComponent(entity);
}

template <typename Component_t>
Component_t const &ecs::ComponentManager::getComponent(Entity_t entity) const
{
    getComponentArray<Component_t>()->getComponent(entity);
}
template <typename Component_t>
std::shared_ptr<ecs::ComponentArray<Component_t>> ecs::ComponentManager::getComponentArray()
{
    char const *name = typeid(Component_t).name();
    assert(m_componentIDs.find(typeName) != m_componentIDs.end() && "component not registered before use");
    std::static_pointer_cast<ComponentArray<Component_t>>(m_componentArrays.at(name));
}
void ecs::ComponentManager::entityDestroyed(Entity_t entity)
{
    for(auto const &pair : m_componentArrays) {
        pair.second->onEntityDestroyed(entity);
    }
}

template <typename System_t>
std::shared_ptr<System_t> ecs::SystemManager::registerSystem()
{
    char const *name = typeid(System_t).name();
    assert(m_systems.find(typeName) == m_systems.end() && "registering system more than once");

    auto system = std::make_shared<System_t>();
    m_systems.insert({name, system});
    return system;
}
template <typename System_t>
void ecs::SystemManager::setSignature(Signature_t signature)
{
    char const *name = typeid(System_t).name();
    assert(mSystems.find(typeName) != mSystems.end() && "system used before registered");
    m_signatures.insert({name, signature});
}

void ecs::SystemManager::entityDestroyed(Entity_t entity)
{
    for(auto const &pair : m_systems) {
        pair.second->m_entities.erase(entity);
    }
}

void ecs::SystemManager::entitySignatureChanged(Entity_t entity, Signature_t signature)
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
