#pragma once
#include <cstdint>
#include <memory>

namespace engine
{
    /// @brief PIMPL / Opaque pointer.
    /// @tparam TObject The type of the implementation class. Usually written as YourClass : public Handle<struct YourClassImpl>
    /// @tparam TPointer The smart pointer type to use to store the implementation pointer.
    /// FIXME: Using shared pointer is probably a wrong choice here.
    template <typename TObject, template<typename> typename TPointer = std::shared_ptr>
    class Handle
    {
    protected:
        TPointer<TObject> mObj = nullptr;
    public:
        inline Handle(TObject *obj) : mObj(obj) {}
        Handle() = default;

        inline TObject &unwrap() { return *mObj; }
        inline TObject const &unwrap() const { return *mObj; }

        inline bool empty() const { return mObj == nullptr; }

        /// @brief two handles are equal if they reference the same object
        /// @return true if both handles are not null and reference the same object.
        bool operator==(Handle const &other) const { return mObj && other.mObj && (*(uint64_t*)mObj == *(uint64_t*)other.mObj); }
        bool operator!=(Handle const &other) const { return !operator==(other); }
    };
} // namespace engine
