/*
      ___  ___ ___ 
     / _ \/ __/ __|    Copyright (c) 2024 Nikita Martynau
    |  __/ (__\__ \    https://opensource.org/license/mit
     \___|\___|___/    https://github.com/nikitawew/ecs  
*/
/**
 * \file ecs.hpp
 * \brief My thread safe (TODO) Entity Component System implementation.
 * 
 * Thanks to this article: https://austinmorlan.com/posts/entity_component_system
 * Took a bit of inspiration from https://github.com/skypjack/entt
 * 
 * The main focus was on the simplicity and small size (one header, < 1000 lines of code).
 * The intended way to use it is by creating the ecs::registry class and using all the functions from it. Only ecs::registry api throws. On errors the rest of implementation will assert.
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
#include <deque>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <typeindex>
#include <string>
#include <atomic>

#include <cassert>
#include "profiler.hpp"
#define ECS_ASSERT(x, msg) assert((x) && (msg))
#define ECS_THROW(x) (throw (x))

#define ECS_CONCAT_DETAIL(A, B) A##B
#define ECS_CONCAT(A, B) ECS_CONCAT_DETAIL(A, B)

#define ECS_LOCK_REGULAR(mutex) std::scoped_lock ECS_CONCAT(lock, __LINE__){mutex}
#define ECS_LOCK_UNIQUE(mutex)  std::unique_lock ECS_CONCAT(lock, __LINE__){mutex}
#define ECS_LOCK_SHARED(mutex)  std::shared_lock ECS_CONCAT(lock, __LINE__){mutex}
// plug-in profiler
#define ECS_PROFILE()

namespace ecs
{
    /**
     * \brief Entity ID.
     */
    using entity = std::uint32_t;
    /**
     * \brief Component ID. Used with signature
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
    using signature = std::bitset<MAX_COMPONENTS>;

    template<typename value_t, typename lock_t>
    class locked
    {
    private:
        value_t m_value;
        lock_t m_lock;
    public:
        locked() = default;
        locked(const locked&) = delete;
        locked &operator=(const locked&) = delete;

        inline operator value_t() const
        {
            return value();
        }
        inline explicit locked(value_t value, lock_t &&lock) : m_value(value), m_lock(std::move(lock)) {};
        inline std::remove_reference_t<value_t> *operator->() const
        {
            return &value();
        }
        inline value_t value() const
        {
            ECS_PROFILE();
            return m_value;
        }
    };
    template<typename lock_t> class locked<void, lock_t> {};
    template<typename value_t> using unique_locked = ecs::locked<value_t, std::unique_lock<std::shared_mutex>>;
    template<typename value_t> using shared_locked = ecs::locked<value_t, std::shared_lock<std::shared_mutex>>;

    /**
     * \brief Manages entities (create, destroy) and their signatures (set, get).
     * Any entity supplied to the manager must be created by the same manager object.
     */
    class entity_manager
    {
    private:
        std::queue<entity> m_availableEntityIDs;
        std::unordered_set<entity> m_entities;
        std::uint32_t m_livingEntitiesCount = 0;
        std::array<signature, MAX_ENTITIES> m_signatures;

        mutable std::shared_mutex m_entitiesMutex;
        mutable std::shared_mutex m_signaturesMutex;
    public:
        entity_manager();
        ~entity_manager() = default;
        /**
         * \brief Creates entity with an optional signature.
         * \param signature A signature representing components the entity has (optional).
         * \return Unique entity id managed by entity_manager.
         */
        entity createEntity(signature signature = {});
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
        void setSignature(entity const &entity, signature signature);

        /**
         * \brief Gets the signature of a valid entity.
         * \param entity A valid entity identifier.
         * \return A copy of a signature, discribing the components an entity has.
         */
        ecs::signature getSignature(entity const &entity) const;
        /**
         * \brief Gets the signature of a valid entity.
         * \param entity A valid entity identifier.
         * \return A locked signature, discribing the components an entity has.
         */
        ecs::unique_locked<ecs::signature &> lockSignature(entity const &entity);

        /**
         * \brief Get entities of this manager.
         * \return A set of valid entities created by this manager
         */
        std::vector<entity> getEntities() const;

        /**
         * \brief Checks if an identifier refers to a valid entity.
         * \param entity An identifier, either valid or not.
         * \return True if the identifier is valid, false otherwise.
         */
        bool valid(entity const &entity) const;
    };

    /**
     * \brief Every instanced component_array is derived from this polymorphic class.
     */
    class icomponent_array 
    {
    public:
        virtual ~icomponent_array() = default;
        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        virtual void onEntityDestroyed(entity const &entity) = 0;
    };

    /**
     * \brief Stores components of entities of a specific type as a sparce set.
     * @tparam component_t The type of stored components.
     */
    template <typename component_t>
    class component_array : public icomponent_array
    {
    private:
        std::vector<component_t> m_components;
        std::unordered_map<entity, size_t> m_entityToIndex;
        std::unordered_map<size_t, entity> m_indexToEntity;

        mutable std::shared_mutex m_componentMutex;
        mutable std::mutex m_mappingMutex;
    public:
        /**
         * \brief Inserts a component to an entity.
         * \param entity A valid entity identifier.
         * \param component A rvalue reference to the component to move.
         */
        void insert(entity const &entity, component_t &&component);

        /**
         * \brief Removes a component of an entity.
         * \param entity A valid entity identifier.
         */
        void remove(entity const &entity);

        /**
         * \brief Gets a component of an entity.
         * \param entity A valid entity identifier.
         * \tparam component_t The component type.
         * \return A locked component lvalue reference.
         */
        ecs::shared_locked<component_t const &> get(entity const &entity) const;
        /**
         * \copydoc get
         */
        ecs::unique_locked<component_t &> lock(entity const &entity);

        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        void onEntityDestroyed(entity const &entity) override;
    };

    /**
     * \brief Manages components and their arrays. All components are destroyed automatically.
     */
    class component_manager
    {
    private:
        std::unordered_map<std::type_index, ComponentID_t> m_componentIDs{};
        std::unordered_map<std::type_index, std::unique_ptr<icomponent_array>> m_componentArrays{};
        std::atomic<ComponentID_t> m_nextID = 0;
        
        mutable std::shared_mutex m_componentsMutex;
    public:
        component_manager() = default;
        ~component_manager() = default;

        /**
         * \brief Registers component.
         * \tparam component_t The component type.
         * This should be called for every component used. Multiple calls for the same component_t will do nothing.
         */
        template <typename component_t> 
        void registerComponent();

        /**
         * \brief Get unique component ID used to index the signature bitset.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        ComponentID_t getComponentID() const;

        /**
         * \brief Adds component to an entity.
         * \param entity A valid entity identifier.
         * \param component An optional rvalue reference to the component to move.
         * \tparam component_t A component type.
         */
        template <typename component_t> 
        void add(entity const &entity, component_t &&component);

        /**
         * \brief Removes component from an entity.
         * \param entity A valid entity identifier.
         * \tparam component_t A component type.
         */
        template <typename component_t> 
        void removeComponent(entity const &entity);

        /**
         * \brief Gets a component of an entity.
         * \param entity A valid entity identifier.
         * \tparam component_t The component type.
         * \return A locked component lvalue reference.
         */
        template <typename component_t> 
        ecs::unique_locked<component_t &> lock(entity const &entity);

        /**
         * \copydoc lock
         */
        template <typename component_t> 
        ecs::shared_locked<component_t const &> get(entity const &entity) const;

        /**
         * \brief Notify component arrays that the entity is destroyed.
         * \param entity A deleted entity identifier.
         */
        void entityDestroyed(entity const &entity) const;
    private:
        template <typename component_t> 
        component_array<component_t> *getComponentArray();
    };

    class registry;

    /**
     * \brief System interface.
     * All systems should derive from that interface.
     */
    class isystem
    {
    public:
        virtual ~isystem() = default;
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
        ecs::entity_manager m_entityManager;
        // ugly fix for lazy component registration
        mutable ecs::component_manager m_componentManager;
        std::unordered_map<std::type_index, std::unique_ptr<isystem>> m_systems{};

        mutable std::mutex m_systemsMutex;
        mutable std::mutex m_entityCreateMutex;
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
         * \brief Constructs and registers a system that implements isystem interface.
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
         * \brief Calls the update callback of every isystem registered.
         */
        void update();

        /**
         * \copydoc ecs::entity_manager::valid
         */
        bool valid(entity const &entity) const;

        /**
         * \brief Checks if a valid entity has a component.
         * \param entity A valid enitiy identifier.
         * \tparam component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the entity has the component, false otherwise.
         */
        template <typename component_t> bool has(entity const &entity) const;

        /**
         * \brief Gets a component from a a valid entity.
         * \param entity A valid enitiy identifier.
         * \tparam component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \return The locked component lvalue reference.
         */
        template <typename component_t> 
        ecs::unique_locked<component_t &> lock(entity const &entity);
        /**
         * \copydoc get
         */
        template <typename component_t> 
        ecs::shared_locked<component_t const &> get(entity const &entity) const;

        /**
         * \brief Removes a component from a valid entity.
         * \param entity A valid enitiy identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        void remove(entity const &entity);
        /**
         * \brief Adds a component from a valid entity.
         * \param entity A valid enitiy identifier.
         * \param component An optional component value to move.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::invalid_argument if the component is already added.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        void add(entity const &entity, component_t &&component = {});

        /**
         * \brief Creates entity with optional components.
         * \tparam Components_t Components  (optional).
         * \return Unique valid entity id managed by entity_manager.
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
        std::vector<entity> getEntities() const;

        /**
         * \copydoc ecs::entity_manager::getSignature
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        ecs::signature getSignature(entity const &entity) const;

        /**
         * \copydoc ecs::entity_manager::
         */
        template <typename component_t> 
        ComponentID_t getComponentID();
    };
} // namespace ecs

// ===============
// Implementation
// ===============

inline ecs::entity_manager::entity_manager()
{
    ECS_PROFILE();
    for(entity id = 1; id <= MAX_ENTITIES; ++id) {
        m_availableEntityIDs.push(id);
    }
}
inline ecs::entity ecs::entity_manager::createEntity(signature signature)
{
    ECS_PROFILE();
    entity entity = 0;
    {
        ECS_LOCK_UNIQUE(m_entitiesMutex);
        ECS_ASSERT(!m_availableEntityIDs.empty(), "too many entities!");
        entity = m_availableEntityIDs.front();
        m_availableEntityIDs.pop();
        ++m_livingEntitiesCount;
        m_entities.emplace(entity);
    }
    ECS_ASSERT(entity != 0, "failed to create entity! (wtf)");
    setSignature(entity, signature);
    return entity;
}
inline void ecs::entity_manager::destroyEntity(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    {
        ECS_LOCK_UNIQUE(m_entitiesMutex);
        --m_livingEntitiesCount;
        m_availableEntityIDs.push(entity);
        m_entities.erase(entity);
    }
    {
        ECS_LOCK_UNIQUE(m_signaturesMutex);
        m_signatures[entity].reset();
    }
}
inline void ecs::entity_manager::setSignature(entity const &entity, signature signature)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    ECS_LOCK_UNIQUE(m_signaturesMutex);
    m_signatures[entity] = signature;
}
inline ecs::signature ecs::entity_manager::getSignature(entity const &entity) const
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    
    ECS_LOCK_SHARED(m_signaturesMutex);
    return m_signatures[entity];
}
inline ecs::unique_locked<ecs::signature &> ecs::entity_manager::lockSignature(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");

    return ecs::unique_locked<ecs::signature &>{m_signatures[entity], std::unique_lock{m_signaturesMutex}};
}
inline bool ecs::entity_manager::valid(entity const &entity) const
{
    ECS_PROFILE();
    ECS_LOCK_SHARED(m_entitiesMutex);
    return 1 <= entity && entity <= MAX_ENTITIES && m_entities.find(entity) != m_entities.end();
}
inline std::vector<ecs::entity> ecs::entity_manager::getEntities() const 
{
    ECS_PROFILE();

    ECS_LOCK_SHARED(m_entitiesMutex);
    return std::vector<ecs::entity>{m_entities.begin(), m_entities.end()};
} 

template <typename component_t>
inline void ecs::component_array<component_t>::insert(entity const &entity, component_t &&component)
{
    ECS_PROFILE();
    ECS_ASSERT(m_entityToIndex.find(entity) == m_entityToIndex.end(), "component added to the same entity more than once!");

    {
        ECS_LOCK_REGULAR(m_mappingMutex);
        size_t index = m_components.size();
        m_entityToIndex[entity] = index;
        m_indexToEntity[index] = entity;
    }
    {
        ECS_LOCK_UNIQUE(m_componentMutex);
        m_components.emplace_back(component);
    }
}
template <typename component_t>
inline void ecs::component_array<component_t>::remove(entity const &entity)
{
    ECS_PROFILE();
    
    ECS_LOCK_REGULAR(m_mappingMutex);
    ECS_LOCK_UNIQUE(m_componentMutex);

    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "removing non-existing component");

    size_t removedEntityIndex = m_entityToIndex[entity];
    size_t lastEntityIndex = m_components.size() - 1;
    
    ecs::entity lastEntity = m_indexToEntity[lastEntityIndex];
    m_entityToIndex[lastEntity] = removedEntityIndex;
    m_indexToEntity[removedEntityIndex] = lastEntity;
    
    m_components[removedEntityIndex] = std::move(m_components[lastEntityIndex]);

    m_components.pop_back();
    m_entityToIndex.erase(entity);
    m_indexToEntity.erase(lastEntityIndex);
}
template <typename component_t>
inline ecs::shared_locked<component_t const &> ecs::component_array<component_t>::get(entity const &entity) const
{
    ECS_PROFILE();
    ECS_LOCK_REGULAR(m_mappingMutex);
    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "retrieving non-existent component");

    auto index = m_entityToIndex.at(entity);
    return ecs::shared_locked<component_t const &>{m_components[index], std::shared_lock{m_componentMutex}};
}
template <typename component_t>
inline ecs::unique_locked<component_t &> ecs::component_array<component_t>::lock(entity const &entity)
{
    ECS_PROFILE();
    ECS_LOCK_REGULAR(m_mappingMutex);
    ECS_ASSERT(m_entityToIndex.find(entity) != m_entityToIndex.end(), "retrieving non-existent component");

    auto index = m_entityToIndex.at(entity);
    return ecs::unique_locked<component_t &>{m_components[index], std::unique_lock{m_componentMutex}};
}
template <typename component_t>
inline void ecs::component_array<component_t>::onEntityDestroyed(entity const &entity)
{
    ECS_PROFILE();
    ECS_LOCK_REGULAR(m_mappingMutex);
    if(m_entityToIndex.find(entity) != m_entityToIndex.end()) {
        remove(entity);
    }   
}

template <typename component_t>
inline void ecs::component_manager::registerComponent()
{
    ECS_PROFILE();
    auto name = std::type_index{typeid(component_t)};
    ECS_LOCK_UNIQUE(m_componentsMutex);
    ECS_ASSERT(m_nextID < MAX_COMPONENTS, "too many components registred!");
    if(m_componentIDs.find(name) != m_componentIDs.end()) {
        return;
    }
    m_componentIDs.insert({name, m_nextID.load()});
    m_componentArrays.insert({name, std::make_unique<component_array<component_t>>()});

    ++m_nextID;
}
template <typename component_t>
inline ecs::ComponentID_t ecs::component_manager::getComponentID() const
{
    ECS_PROFILE();
    auto name = std::type_index{typeid(component_t)};
    ECS_LOCK_SHARED(m_componentsMutex);
    ECS_ASSERT(m_componentIDs.find(name) != m_componentIDs.end(), "component not registered before use");
    return m_componentIDs.at(name);
}
template <typename component_t>
inline void ecs::component_manager::add(entity const &entity, component_t &&component)
{
    ECS_PROFILE();
    getComponentArray<component_t>()->insert(entity, std::forward<component_t>(component));
}
template <typename component_t>
inline void ecs::component_manager::removeComponent(entity const &entity)
{
    ECS_PROFILE();
    getComponentArray<component_t>()->remove(entity);
}
template <typename component_t>
inline ecs::unique_locked<component_t &> ecs::component_manager::lock(entity const &entity)
{
    ECS_PROFILE();
    return getComponentArray<component_t>()->lock(entity);
}
template <typename component_t>
inline ecs::shared_locked<component_t const &> ecs::component_manager::get(entity const &entity) const
{
    ECS_PROFILE();
    return getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
inline ecs::component_array<component_t> *ecs::component_manager::getComponentArray()
{
    ECS_PROFILE();
    auto name = std::type_index{typeid(component_t)};
    ECS_LOCK_SHARED(m_componentsMutex);
    ECS_ASSERT(m_componentIDs.find(name) != m_componentIDs.end(), "component not registered before use");
    return static_cast<component_array<component_t> *>(m_componentArrays.at(name).get());
}
inline void ecs::component_manager::entityDestroyed(entity const &entity) const
{
    ECS_PROFILE();
    ECS_LOCK_SHARED(m_componentsMutex);
    for(auto const &[name, componentArray] : m_componentArrays) {
        componentArray->onEntityDestroyed(entity);
    }
}

template <typename System>
inline System *ecs::registry::addSystem()
{
    ECS_PROFILE();
    auto name = std::type_index{typeid(System)};
    ECS_LOCK_REGULAR(m_systemsMutex);
    if(m_systems.find(name) != m_systems.end()) ECS_THROW(std::invalid_argument{"system added more than once!"});

    m_systems[name] = std::make_unique<System>();
    return m_systems.at(name).get();
}
template <typename System>
inline void ecs::registry::removeSystem()
{
    ECS_PROFILE();
    auto name = std::type_index{typeid(System)};
    ECS_LOCK_REGULAR(m_systemsMutex);
    if(m_systems.find(name) == m_systems.end()) ECS_THROW(std::out_of_range{"system not registered before use1"});
    m_systems.erase(name);
}
template <typename component_t> 
inline bool ecs::registry::has(entity const &entity) const
{ 
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<component_t>();
    return getSignature(entity).test(m_componentManager.getComponentID<component_t>()); 
}
template <typename component_t>
inline ecs::unique_locked<component_t &> ecs::registry::lock(entity const &entity) 
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<component_t>();
    if(!has<component_t>(entity)) ECS_THROW(std::out_of_range{"component to get is not added!"});
    return m_componentManager.lock<component_t>(entity);
}
template <typename component_t>
inline ecs::shared_locked<component_t const &> ecs::registry::get(entity const &entity) const
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<component_t>();
    if(!has<component_t>(entity)) ECS_THROW(std::out_of_range{"component to get is not added!"});
    return m_componentManager.get<component_t>(entity);
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create()
{
    ECS_PROFILE();
    ECS_LOCK_REGULAR(m_entityCreateMutex);
    (m_componentManager.registerComponent<Components_t>(), ...);
    signature signature;
    (signature.set(m_componentManager.getComponentID<Components_t>()), ...);
    entity entity = m_entityManager.createEntity(signature);
    (m_componentManager.add(entity, Components_t{}), ...);
    return entity;
}
template <typename component_t> 
inline void ecs::registry::remove(entity const &entity) 
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to remove is not added!"});
    m_componentManager.removeComponent<component_t>(entity);
    m_entityManager.lockSignature(entity)->set(m_componentManager.getComponentID<component_t>(), false);
}
template <typename component_t>
inline void ecs::registry::add(entity const &entity, component_t &&component)
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_componentManager.registerComponent<component_t>();
    if(has<component_t>(entity)) ECS_THROW(std::invalid_argument{"component to add already added!"});
    m_componentManager.add<component_t>(entity, std::forward<component_t>(component));
    m_entityManager.lockSignature(entity)->set(m_componentManager.getComponentID<component_t>(), true);
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
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    m_entityManager.destroyEntity(entity);
    m_componentManager.entityDestroyed(entity);
}
inline std::vector<ecs::entity> ecs::registry::getEntities() const
{
    ECS_PROFILE();
    return m_entityManager.getEntities();
}
template<typename Type, typename... Other, typename... Exclude>
inline std::vector<ecs::entity> ecs::registry::view(exclude_t<Exclude...>) const
{
    m_componentManager.registerComponent<Type>();
    (m_componentManager.registerComponent<Other>(), ...);
    (m_componentManager.registerComponent<Exclude>(), ...);
    ECS_PROFILE();
    signature required;
    required.set(m_componentManager.getComponentID<Type>());
    (required.set(m_componentManager.getComponentID<Other>()), ...);
    signature excluded;
    (excluded.set(m_componentManager.getComponentID<Exclude>()), ...);

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
inline ecs::signature ecs::registry::getSignature(entity const &entity) const
{
    ECS_PROFILE();
    return m_entityManager.getSignature(entity);
}
template <typename component_t> 
inline ecs::ComponentID_t ecs::registry::getComponentID()
{
    ECS_PROFILE();
    m_componentManager.registerComponent<component_t>();
    return m_componentManager.getComponentID<component_t>();
}
