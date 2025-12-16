#pragma once

namespace engine
{
    /// @brief PIMPL / Opaque pointer.
    template <typename TObject>
    class Handle
    {
    protected:
        TObject* mObj;
    public:
        Handle() = default;
        Handle(TObject* obj) : mObj(obj) {}

        inline TObject *unwrap() { return mObj; }
        inline TObject const *unwrap() const { return mObj; }

        /// @brief two handles are equal if they reference the same object
        /// @return true if both handles are not null and reference the same object.
        bool operator==(Handle const &other) const { return mObj && other.mObj && (*(uint64_t*)mObj == *(uint64_t*)other.mObj); }
        bool operator!=(Handle const &other) const { return !operator==(other); }
    };
} // namespace engine
