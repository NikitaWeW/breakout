#pragma once
#include <cstddef>
#include <string>
#include <algorithm>

namespace engine
{

/// @brief Add a hash value @p v to a @p seed
inline void hashCombine(size_t &seed, size_t const &v)
{
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace engine
