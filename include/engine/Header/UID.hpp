#pragma once
#include <atomic>
#include <vector> // For hash declaration

namespace engine
{
    class UID
    {
    public:
        using type = uint64_t;
    private:
        inline static std::atomic<type> m_next = 0;
        type m_id;
    public:
        inline UID() : m_id(m_next++) {};
        inline type value() const { return m_id; };

        friend bool operator==(UID const &lhs, UID const &rhs) { return lhs.m_id == rhs.m_id; }
        friend bool operator!=(UID const &lhs, UID const &rhs) { return lhs.m_id != rhs.m_id; }
        friend bool operator< (UID const &lhs, UID const &rhs) { return lhs.m_id <  rhs.m_id; }
        friend bool operator<=(UID const &lhs, UID const &rhs) { return lhs.m_id <= rhs.m_id; }
        friend bool operator> (UID const &lhs, UID const &rhs) { return lhs.m_id >  rhs.m_id; }
        friend bool operator>=(UID const &lhs, UID const &rhs) { return lhs.m_id >= rhs.m_id; }
    };
} // namespace engine

namespace std {
    template <>
    struct hash<engine::UID> {
        inline size_t operator()(engine::UID const &uid) const 
        {
            return hash<engine::UID::type>{}(uid.value()); 
        }
    };
} // namespace std