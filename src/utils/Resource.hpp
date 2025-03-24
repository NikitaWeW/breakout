#pragma once

/*
deallocate only if canDeallocate returns true
*/
class Resource 
{
private:
    mutable bool m_managing = false;
public:
    Resource();
    Resource(Resource const &other);
    Resource(Resource &&other);
    void operator=(Resource const &other);
    void operator=(Resource &&other);

    bool canDeallocate() const;
    ~Resource() = default;
};