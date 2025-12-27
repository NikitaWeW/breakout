#pragma once

#include "engine/Header/Config.hpp"
#include "nicecs/ecs.hpp"
#include <shared_mutex>

namespace engine
{

/// @brief A thin layer above the ecs::registry with support for thread synchronization.
class Registry : public ecs::registry
{
private:
    mutable std::shared_mutex mMutex;
public:
    // Registry() = default;
    using ecs::registry::registry;

    /// @brief Get the shared mutex.
    inline std::shared_mutex &getMutex() const { return mMutex; }
};

/// @brief A helper class to group the entity and the registry it belongs to.
/// Invalidates if the registry is moved or if the entity invalidates obviously.
struct Entity
{
private:
    // FIXME: A safer approach is needed. use std::weak_ptr maybe?
    Registry *mReg = nullptr;
    ecs::entity mEntity = 0;
public:
    inline explicit Entity() = default;
    inline explicit Entity(Registry &reg) : mReg(&reg), mEntity(mReg->create<>()) {}
    inline explicit Entity(Registry &reg, ecs::entity e) : mReg(&reg), mEntity(e) {}
    inline ecs::entity entity() const { return mEntity; }
    inline Registry const &reg() const 
    { 
        ENGINE_ASSERT_MSG(mReg, "Invalid registry!");
        return *mReg; 
    }
    inline Registry &reg() 
    { 
        ENGINE_ASSERT_MSG(mReg, "Invalid registry!");
        return *mReg; 
    }

    template <typename component_t, class... Args>
    inline void emplace(Args&&... args) { return reg().emplace<component_t, Args...>(entity(), std::forward<Args>(args)...); }

    /// @copydoc ecs::registry::has
    template <typename component_t> 
    inline bool has() const { return reg().has<component_t>(entity()); }

    /// @copydoc ecs::registry::get
    template <typename component_t> 
    inline component_t const &get() const { return reg().get<component_t>(entity()); }
    /// @copydoc ecs::registry::get
    template <typename component_t> 
    inline component_t &get() { return reg().get<component_t>(entity()); }

    /// @copydoc ecs::registry::remove
    template <typename component_t> 
    inline void remove() { return reg().remove<component_t>(entity()); }

    /// @copydoc ecs::registry::size
    inline std::size_t size() const { return reg().size(entity()); }

    /// @copydoc ecs::registry::valid
    /// Also checks if the registry is valid (not nullptr)
    inline bool valid() const { return mReg && reg().valid(entity()); }
};

/// @brief Transform a registry and a list of entities into the list of engine::Entity's
template<template<typename> typename ContainerT = std::vector>
constexpr ContainerT<Entity> toEntities(Registry &reg, ContainerT<ecs::entity> const &entities)
{
    ContainerT<Entity> res;
    for(auto const &e : entities)
        res.emplace_back(reg, e);
    return res;
}

constexpr ecs::entity INVALID_ENTITY = 0u;
} // namespace engine
