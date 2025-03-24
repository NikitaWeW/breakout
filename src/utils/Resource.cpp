#include "Resource.hpp"
#include <utility>

Resource::Resource() : m_managing(true) {}
Resource::Resource(Resource const &other)       { m_managing = other.m_managing; other.m_managing = false; }
Resource::Resource(Resource &&other)            { m_managing = other.m_managing; other.m_managing = false; }
void Resource::operator=(Resource const &other) { m_managing = other.m_managing; other.m_managing = false; }
void Resource::operator=(Resource &&other)      { m_managing = other.m_managing; other.m_managing = false; }

bool Resource::canDeallocate() const
{
    return m_managing;
}
