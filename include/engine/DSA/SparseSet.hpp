#pragma once
#include "ECS.hpp"

namespace engine
{

/// @copydoc ecs::sparse_set
template<typename T>
using SparseSet = ecs::sparse_set<T>;

constexpr auto SPARSE_SET_NULL = ecs::SPARSE_SET_NULL;

} // namespace engine
