/*
      ___  ___ ___ 
     / _ \/ __/ __|    Copyright (c) 2024 Nikita Martynau
    |  __/ (__\__ \    https://opensource.org/license/mit
     \___|\___|___/    https://github.com/nikitawew/ecs  
*/
/**
 * \file ecs.hpp
 * \brief My thread safe (TODO) Entity Component System implimentation.
 * 
 * Thanks to this article: https://austinmorlan.com/posts/entity_component_system
 * Took a bit of inspiration from https://github.com/skypjack/entt
 * 
 * The main focus was on the simplicity and small size (one header, < 1000 lines of code).
 * The intended way to use it is by creating the ecs::registry class and using all the functions from it.
 */
/*
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <cassert>
#include "profiler.hpp"
#define ECS_ASSERT(x, msg) assert((x) && (msg))
#define ECS_THROW(x) (throw (x))
#define ECS_LOCK(mutex) (std::scoped_lock lock##__LINE__{mutex})
// plug-in profiler
#define ECS_PROFILE()

namespace ecs
{
    /**
     * \brief Entity ID.
     */
    using entity = std::uint32_t;
    /**
     * \brief Component ID. Used with Signature_t
     */
    using ComponentID_t = std::uint8_t;

    /**
     * \brief Controls the maximum number of entities allowed to exist simultaneously.
     */
    const entity MAX_ENTITIES = 5000;
    /**
     * \brief Controls the maximum number of registered components allowed to exist simultaneously.
     */
    const ComponentID_t MAX_COMPONENTS = 128;

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
        std::queue<entity> m_availableEntityIDs;
        std::unordered_set<entity> m_aliveEntities;
        std::uint32_t m_livingEntitiesCount = 0;
        std::array<Signature_t, MAX_ENTITIES> m_signatures;
    public:
        EntityManager();
        ~EntityManager() = default;
        /**
         * \brief Creates entity with an optional signature.
         * \param signature A signature representing components the entity has (optional).
         * \return Unique entity id managed by EntityManager.
         */
        entity createEntity(Signature_t signature = {});
        /**
         * \brief Destroys entity.
         * \param entity A valid entity identifier.
         */
        void destroyEntity(entity const &entity);
        /**
         * \brief Sets the signature of the entity.
         * \param entity A valid entity identifier.
         * \param signature A new signature,
         */
        void setSignature(entity const &entity, Signature_t signature);

        /**
         * \brief Gets the signature of a valid entity.
         * \param entity A valid entity identifier.
         * \return A signature discribing the components an entity has.
         */
        Signature_t const &getSignature(entity const &entity) const;
        /**
         * \copydoc getSignature
         */
        Signature_t &getSignature(entity const &entity);

        /**
         * \brief Get entities of this manager.
         * \return A set of valid entities created by this manager
         */
        std::unordered_set<entity> const &getEntities() const;

        /**
         * \brief Checks if an identifier refers to a valid entity.
         * \param entity An identifier, either valid or not.
         * \return True if the identifier is valid, false otherwise.
         */
        bool valid(entity const &entity) const;
    };

    /**
     * \brief Every instanced ComponentArray is derived from this polymorphic class.
     */
    class IComponentArray 
    {
    public:
        virtual ~IComponentArray() = default;
        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        virtual void onEntityDestroyed(entity const &entity) = 0;
    };

    /**
     * \brief Stores components of entities of a specific type as a sparce set.
     * @tparam Component_t The type of stored components.
     */
    template <typename Component_t>
    class ComponentArray : public IComponentArray
    {
    private:
        std::vector<Component_t> m_components{};
        std::unordered_map<entity, size_t> m_entityToIndex{};
        std::unordered_map<size_t, entity> m_indexToEntity{};
    public:
        /**
         * \brief Inserts a component to an entity.
         * \param entity A valid entity identifier.
         * \param component A rvalue reference to the component to move.
         */
        void insert(entity const &entity, Component_t &&component);

        /**
         * \brief Removes a component of an entity.
         * \param entity A valid entity identifier.
         */
        void remove(entity const &entity);

        /**
         * \brief Gets a component of an entity.
         * \param entity A valid entity identifier.
         * \return A component lvalue reference.
         */
        Component_t const &getComponent(entity const &entity) const;
        /**
         * \copydoc getComponent
         */
        Component_t &getComponent(entity const &entity);

        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        void onEntityDestroyed(entity const &entity) override;
    };

    /**
     * \brief Manages components and their arrays. All components are destroyed automatically.
     */
    class ComponentManager
    {
    private:
        std::unordered_map<char const *, ComponentID_t> m_componentIDs{};
        std::unordered_map<char const *, std::unique_ptr<IComponentArray>> m_componentArrays{};
        ComponentID_t m_nextID = 0;
    public:
        ComponentManager() = default;
        ~ComponentManager() = default;

        /**
         * \brief Registers component.
         * \tparam Component_t The component type.
         * This should be called for every component used. Multiple calls for the same Component_t will do nothing.
         */
        template <typename Component_t> 
        void registerComponent();
        /**
         * \brief Get unique component ID used to index the Signature_t bitset.
         * \tparam Component_t The component type.
         */
        template <typename Component_t> 
        ComponentID_t getComponentID();

        /**
         * \brief Adds component to an entity.
         * \param entity A valid entity identifier.
         * \param component An optional rvalue reference to the component to move.
         * \tparam Component_t A component type.
         */
        template <typename Component_t> 
        void addComponent(entity const &entity, Component_t &&component);

        /**
         * \brief Removes component from an entity.
         * \param entity A valid entity identifier.
         * \tparam Component_t A component type.
         */
        template <typename Component_t> 
        void removeComponent(entity const &entity);

        /**
         * \brief Gets an entity component.
         * \param entity A valid entity identifier.
         * \tparam Component_t A component type.
         * \return A component lvalue reference.
         */
        template <typename Component_t> 
        Component_t &getComponent(entity const &entity);

        /**
         * \copydoc getComponent
         */
        template <typename Component_t> 
        Component_t const &getComponent(entity const &entity) const;

        /**
         * \brief Notify component arrays that the entity is destroyed.
         * \param entity A deleted entity identifier.
         */
        void entityDestroyed(entity const &entity) const;
    private:
        template <typename Component_t> 
        ComponentArray<Component_t> *getComponentArray();
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
        virtual void update(registry &registry) = 0;
    };

    /**
     * \brief An ECS interface.
     * This class should be enough to use the ecs.
     */
    class registry
    {
    private:
        EntityManager m_entityManager;
        ComponentManager m_componentManager;
        std::unordered_map<std::string_view, std::unique_ptr<ISystem>> m_systems{};
    public:
        /**
         * \brief Alias for exclusion lists.
         * \tparam Type List of types.
         * From the entt lib.
         */
        template<typename... Type>
        struct exclude_t final {
            /** \brief Default constructor. */
            explicit constexpr exclude_t() = default;
            /** \brief Type Types provided by the type list. */
            using type = exclude_t;
            /** \brief Compile-time number of elements in the type list. */
            static constexpr auto size = sizeof...(Type);
        };


        /**
         * \brief Constructs and registers a system that implements ISystem interface.
         * \tparam System The system class.
         * \throws std::invalid_argument If the same system is added more than once.
         * \return A pointer to a construced system.
         */
        template <typename System>
        System *addSystem();
        /**
         * \brief Removes and deallocates a system added with addSystem.
         * \tparam System The system class.
         * \throws std::out_of_range If the system to remove is not added.
         */
        template <typename System>
        void removeSystem();

        /**
         * \brief Calls the update callback of every ISystem registered.
         */
        void update();

        /**
         * \copydoc ecs::EntityManager::valid
         */
        bool valid(entity const &entity) const;

        /**
         * \brief Checks if a valid entity has a component.
         * \param entity A valid enitiy identifier.
         * \tparam Component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the entity has the component, false otherwise.
         */
        template <typename Component_t> bool has(entity const &entity) const;

        /**
         * \brief Gets a component from a a valid entity.
         * \param entity A valid enitiy identifier.
         * \tparam Component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \return The component lvalue reference.
         */
        template <typename Component_t> 
        Component_t &get(entity const &entity);
        /**
         * \copydoc ecs::registry::get
         */
        template <typename Component_t> 
        Component_t const &get(entity const &entity) const;

        /**
         * \brief Removes a component from a valid entity.
         * \param entity A valid enitiy identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \tparam Component_t The component type.
         */
        template <typename Component_t> 
        void remove(entity const &entity);
        /**
         * \brief Adds a component from a valid entity.
         * \param entity A valid enitiy identifier.
         * \param component An optional component value to move.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::invalid_argument if the component is already added.
         * \tparam Component_t The component type.
         */
        template <typename Component_t> 
        void add(entity const &entity, Component_t &&component = {});

        /**
         * \brief Creates entity with optional components.
         * \tparam Components_t Components  (optional).
         * \return Unique valid entity id managed by EntityManager.
         */
        template <typename... Components_t> 
        ecs::entity create();

        /**
         * \brief Destroys an entity and its components.
         * \param entity A valid enitiy identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        void destroy(ecs::entity const &entity);

        /**
         * \brief Returns a view for the given elements.
         * \tparam Type Type of element used to construct the view.
         * \tparam Other Other types of elements used to construct the view.
         * \tparam Exclude Types of elements used to filter the view.
         * \return A newly created view.
         */
        template<typename Type, typename... Other, typename... Exclude>
        std::vector<entity> view(exclude_t<Exclude...> = exclude_t{}) const;

        /**
         * \brief Get entities of this registry.
         * \return A set of valid entities created by this registry.
         */
        std::unordered_set<entity> const &getEntities() const;

        /**
         * \copydoc ecs::EntityManager::getSignature
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        Signature_t const &getSignature(entity const &entity) const;

        /**
         * \copydoc ecs::EntityManager::
         */
        template <typename Component_t> 
        ComponentID_t getComponentID();
    };
} // namespace ecs


// ===============
// Implementation
// ===============

inline ecs::EntityManager::EntityManager()
{
    ECS_PROFILE();
    for(entity id = 1; id < MAX_ENTITIES; ++id) {
        m_availableEntityIDs.push(id);
    }
}
inline ecs::entity ecs::EntityManager::createEntity(Signature_t signature)
{
    ECS_PROFILE();
    ECS_ASSERT(!m_availableEntityIDs.empty(), "too many entities!");
    entity entity = m_availableEntityIDs.front();
    m_availableEntityIDs.pop();
    ++m_livingEntitiesCount;
    m_aliveEntities.emplace(entity);
    setSignature(entity, signature);
    return entity;
}
inline void ecs::EntityManager::destroyEntity(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);
    m_aliveEntities.erase(entity);

    m_signatures.at(entity).reset();
}
inline void ecs::EntityManager::setSignature(entity const &entity, Signature_t signature)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    m_signatures[entity] = signature;
}
inline ecs::Signature_t const &ecs::EntityManager::getSignature(entity const &entity) const
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    return m_signatures.at(entity); 
}
inline ecs::Signature_t &ecs::EntityManager::getSignature(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    return m_signatures.at(entity);
}

inline bool ecs::EntityManager::valid(entity const &entity) const
{
    ECS_PROFILE();
    return 1 <= entity && entity < MAX_ENTITIES && m_aliveEntities.find(entity) != m_aliveEntities.end();
}

template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::insert(entity const &entity, Component_t &&component)
{
    ECS_PROFILE();
    ECS_ASSERT(m_entityToIndex.find(entity) == m_entityToIndex.end(), "component added to the same entity more than once!");

    size_t index = m_components.size();
    m_entityToIndex[entity] = index;
    m_indexToEntity[index] = entity;
    m_components.emplace_back(component);
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::remove(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "removing non-existing component");
    size_t removedEntityIndex = m_entityToIndex.at(entity);
    size_t lastEntityIndex = m_components.size() - 1;
    m_components[removedEntityIndex] = std::move(m_components[lastEntityIndex]);

    entity lastEntity = m_indexToEntity.at(lastEntityIndex);
    m_entityToIndex.at(lastEntity) = removedEntityIndex;
    m_indexToEntity.at(removedEntityIndex) = lastEntity;

    m_components.pop_back();
    m_entityToIndex.erase(entity);
    m_indexToEntity.erase(lastEntityIndex);
}
template <typename Component_t>
inline Component_t const &ecs::ComponentArray<Component_t>::getComponent(entity const &entity) const
{
    ECS_PROFILE();
    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline Component_t &ecs::ComponentArray<Component_t>::getComponent(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "retrieving non-existent component");

    return m_components.at(m_entityToIndex.at(entity));
}
template <typename Component_t>
inline void ecs::ComponentArray<Component_t>::onEntityDestroyed(entity const &entity)
{
    ECS_PROFILE();
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        remove(entity);
    }   
}

template <typename Component_t>
inline void ecs::ComponentManager::registerComponent()
{
    ECS_PROFILE();
    ECS_ASSERT(m_nextID < MAX_COMPONENTS, "too many components registred!");
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
    ECS_PROFILE();
    char const *name = typeid(Component_t).name();
    ECS_ASSERT(m_componentIDs.find(name) != m_componentIDs.end(), "component not registered before use");
    return m_componentIDs.at(name);
}
template <typename Component_t>
inline void ecs::ComponentManager::addComponent(entity const &entity, Component_t &&component)
{
    ECS_PROFILE();
    getComponentArray<Component_t>()->insert(entity, std::forward<Component_t>(component));
}
template <typename Component_t>
inline void ecs::ComponentManager::removeComponent(entity const &entity)
{
    ECS_PROFILE();
    getComponentArray<Component_t>()->remove(entity);
}
template <typename Component_t>
inline Component_t &ecs::ComponentManager::getComponent(entity const &entity)
{
    ECS_PROFILE();
    return getComponentArray<Component_t>()->getComponent(entity);
}

template <typename Component_t>
inline Component_t const &ecs::ComponentManager::getComponent(entity const &entity) const
{
    ECS_PROFILE();
    return getComponentArray<Component_t>()->getComponent(entity);
}
template <typename Component_t>
inline ecs::ComponentArray<Component_t> *ecs::ComponentManager::getComponentArray()
{
    ECS_PROFILE();
    char const *name = typeid(Component_t).name();
    ECS_ASSERT(m_componentIDs.find(name) != m_componentIDs.end(), "component not registered before use");
    return dynamic_cast<ComponentArray<Component_t> *>(m_componentArrays.at(name).get());
}
inline void ecs::ComponentManager::entityDestroyed(entity const &entity) const
{
    for(auto const &[name, componentArray] : m_componentArrays) {
        componentArray->onEntityDestroyed(entity);
    }
}

template <typename System>
inline System *ecs::registry::addSystem()
{
    ECS_PROFILE();
    char const *name = typeid(System_t).name();
    if(m_systems.find(name) != m_systems.end()) ECS_THROW(std::invalid_argument{"system added more than once!"});

    auto system = std::make_shared<System_t>();
    m_systems.insert({name, system});
    return system;
}
template <typename System_t>
inline void ecs::registry::removeSystem()
{
    ECS_PROFILE();
    char const *name = typeid(System_t).name();
    if(m_systems.find(name) == m_systems.end()) ECS_THROW(std::out_of_range{"system not registered before use1"});
    m_systems.erase(name);
}
template <typename Component_t> 
inline bool ecs::registry::has(entity const &entity) const
{ 
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    return getSignature(entity)[m_componentManager.getComponentID<Component_t>()]; 
}
template <typename Component_t>
inline Component_t &ecs::registry::get(entity const &entity) 
{
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<Component_t>();
    if(!has<Component_t>(entity)) ECS_THROW(std::out_of_range{"component to get is not added!"});
    return m_componentManager.getComponent<Component_t>(entity);
}
template <typename Component_t>
inline Component_t const &ecs::registry::get(entity const &entity) const
{
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<Component_t>();
    if(!has<Component_t>(entity)) ECS_THROW(std::out_of_range{"component to get is not added!"});
    return m_componentManager.getComponent<Component_t>(entity);
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create()
{
    ECS_PROFILE();
    (m_componentManager.registerComponent<Components_t>(), ...);
    Signature_t signature;
    (signature.set(m_componentManager.getComponentID<Components_t>()), ...);
    entity entity = m_entityManager.createEntity(signature);
    (m_componentManager.addComponent(entity, Components_t{}), ...);
    return entity;
}
template <typename Component_t> 
void ecs::registry::remove(entity const &entity) 
{
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<Component_t>();
    if(!has<Component_t>(entity)) ECS_THROW(std::out_of_range{"component to remove is not added!"});
    m_componentManager.removeComponent<Component_t>(entity);
    getSignature(entity).set(m_componentManager.getComponentID<Component_t>(), false);
}
template <typename Component_t>
void ecs::registry::add(entity const &entity, Component_t &&component)
{
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<Component_t>();
    if(has<Component_t>(entity)) ECS_THROW(std::invalid_argument{"component to add already added!"});
    m_componentManager.addComponent<Component_t>(entity, std::forward<Component_t>(component));
    getSignature(entity).set(m_componentManager.getComponentID<Component_t>(), true);
}
inline void ecs::registry::update()
{
    ECS_PROFILE();
    for(auto &[name, system] : m_systems)
    {
        system->update(*this);
    }
}
inline bool ecs::registry::valid(entity const &entity) const
{
    ECS_PROFILE();
    return m_entityManager.valid(entity);
}
inline void ecs::registry::destroy(ecs::entity const &entity)
{
    ECS_PROFILE();
    if(!valid(entity)) ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_entityManager.destroyEntity(entity);
    m_componentManager.entityDestroyed(entity);
}
inline std::unordered_set<ecs::entity> const &ecs::registry::getEntities() const
{
    ECS_PROFILE();
    return m_entityManager.getEntities();
}
template<typename Type, typename... Other, typename... Exclude>
std::vector<ecs::entity> ecs::registry::view(exclude_t<Exclude...> = exclude_t{}) const
{
    m_componentManager.registerComponent<Type>();
    (m_componentManager.registerComponent<Other>(), ...);
    (m_componentManager.registerComponent<Exclude>(), ...);
    ECS_PROFILE();
    Signature_t required = m_componentManager.getComponentID<Type>() | (m_componentManager.getComponentID<Other>() | ...);
    Signature_t excluded = (m_componentManager.getComponentID<Exclude>() | ...);

    std::vector<ecs::entity> result;
    result.reserve(10);
    for(auto const &entity : getEntities())
    {
        auto signature = getSignature(entity);
        if((signature & required) == required && (signature & excluded).none())
            result.emplace_back(entity);
    }

    return result;
}
ecs::Signature_t const &ecs::registry::getSignature(entity const &entity) const
{
    ECS_PROFILE();
    return m_entityManager.getSignature(entity);
}
template <typename Component_t> 
ecs::ComponentID_t ecs::registry::getComponentID()
{
    ECS_PROFILE();
    m_componentManager.registerComponent<Component_t>();
    return m_componentManager.getComponentID();
}
