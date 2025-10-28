#pragma once
#include <atomic>

namespace engine
{
    class UID
    {
    public:
        using type = unsigned long long;
    private:
        inline static std::atomic<type> m_next = 0;
        type m_id;
    public:
        inline UID() : m_id(m_next++) {};
        inline type get() const { return m_id; };
    };
} // namespace engine
