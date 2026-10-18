// =============================================================================
// Handle.hpp
//
// A lightweight typed handle to an object living in a memory container.
//
// TODO: change to 32-bit where 20 bits are index and 12 bits are generation
// =============================================================================

#pragma once
#include <cstddef>    // size_t
#include <cstdint>    // uint32_t, uint64_t
#include <functional> // hash

namespace Nezumi {

template <typename T>
class Handle {
public:

    // -------------------------------------------------------------------------
    // Types
    // -------------------------------------------------------------------------

    using Type    = T;
    using Index   = std::uint32_t;
    using Version = std::uint32_t;

    static constexpr Index   NULL_INDEX   = ~Index{0};
    static constexpr Version NULL_VERSION = 0;

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    constexpr Handle() noexcept = default;

    constexpr Handle(Index index, Version version) noexcept
        : index_(index), version_(version) {}

    // -------------------------------------------------------------------------
    // Validity
    // -------------------------------------------------------------------------

    constexpr bool is_null() const noexcept {
        return index_ == NULL_INDEX;
    }

    constexpr explicit operator bool() const noexcept { return !is_null(); }

    // -------------------------------------------------------------------------
    // Comparison (C++20 defaulted: also synthesizes operator!=)
    // -------------------------------------------------------------------------

    constexpr bool operator==(const Handle& other) const noexcept = default;

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    constexpr Index   index()   const noexcept { return index_;   }
    constexpr Version version() const noexcept { return version_; }

    constexpr std::uint64_t raw() const noexcept {
        return (static_cast<std::uint64_t>(index_) << 32) |
                static_cast<std::uint64_t>(version_);
    }

private:

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Index   index_   = NULL_INDEX;   // Index in the container
    Version version_ = NULL_VERSION; // Generation counter
};

} // namespace Nezumi

// -----------------------------------------------------------------------------
// Hash support for use in std containers
// -----------------------------------------------------------------------------

template <typename T>
struct std::hash<Nezumi::Handle<T>> {
    size_t operator()(const Nezumi::Handle<T>& k) const noexcept {
        return std::hash<uint64_t>{}(k.raw());
    }
};
