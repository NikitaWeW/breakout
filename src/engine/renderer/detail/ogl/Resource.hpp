#pragma once

#include <memory>
#include <atomic>

/**
 * @brief Base class representing a reference-counted resource.
 *
 * Objects of this class maintain an internal atomic reference count
 * and manage deallocation only when it is safe to do so.
 */
class Resource 
{
private:
    /**
     * @brief Internal atomic reference count.
     *
     * This count starts at 1 and tracks the number of active references
     * to the resource. Deallocation should only occur when this value
     * indicates no outstanding owners and canDeallocate() returns true.
     */
    mutable std::atomic_uint m_referenceCount = 1;

public:
    /**
     * @brief Default constructor.
     *
     * Initializes the resource with a reference count of 1.
     */
    Resource();

    /**
     * @brief Copy constructor.
     *
     * Creates a new Resource referencing the same underlying data.
     * Increments the internal reference count to reflect the new owner.
     *
     * @param other The Resource to copy from.
     */
    Resource(Resource const &other);

    /**
     * @brief Move constructor.
     *
     * Transfers ownership from the given Resource to the new instance.
     * Does not increase the reference count, but leaves the source in a
     * valid, empty state if applicable.
     *
     * @param other The Resource to move from.
     */
    Resource(Resource &&other);

    /**
     * @brief Copy assignment operator.
     *
     * Releases the current ownership (decrementing the reference count
     * and possibly deallocating if safe), then copies from @p other,
     * incrementing its reference count.
     *
     * @param other The Resource to copy from.
     */
    void operator=(Resource const &other);

    /**
     * @brief Move assignment operator.
     *
     * Releases the current ownership, then acquires ownership from
     * @p other without incrementing the reference count.
     *
     * @param other The Resource to move from.
     */
    void operator=(Resource &&other);

    /**
     * @brief Checks if the resource can be safely deallocated.
     *
     * This function should return true only when the resource
     * has no remaining dependents and all conditions for deallocation
     * are satisfied.
     *
     * @return True if the resource can be deallocated; false otherwise.
     */
    bool canDeallocate() const;

    /**
     * @brief Virtual destructor.
     *
     * Decrements the reference count and deallocates the resource
     * if no references remain and canDeallocate() returns true.
     */
    virtual ~Resource() = default;
};
