// =============================================================================
// ID.hpp
//
// Typed object identification.
// =============================================================================

#pragma once
#include <concepts>   // unsigned_integral
#include <cstddef>    // size_t
#include <cstdint>    // uint32_t
#include <functional> // hash
#include <limits>     // numeric_limits

namespace Nezumi {

template<typename TObject, std::unsigned_integral TValue = std::uint32_t>
class ID {
public:

    // -------------------------------------------------------------------------
    // Statics
    // -------------------------------------------------------------------------

    using value_t = TValue;
    static constexpr TValue INVALID_VALUE = std::numeric_limits<TValue>::max();
    static constexpr TValue max_value() { return INVALID_VALUE; }

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    constexpr ID() = default;
    constexpr ID(TValue value) noexcept : value_(value) {}

    static constexpr ID null() noexcept { return ID{}; }

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    constexpr bool is_valid() const noexcept { return value_ != INVALID_VALUE; }
    explicit constexpr operator bool() const noexcept { return is_valid(); }

    // -------------------------------------------------------------------------
    // Comparison
    // -------------------------------------------------------------------------

    constexpr bool operator== (const ID&) const noexcept = default;
    constexpr bool operator!= (const ID&) const noexcept = default;
    constexpr auto operator<=>(const ID&) const noexcept = default;

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    constexpr TValue value() const noexcept { return value_; }

private:
    TValue value_ = INVALID_VALUE;
};

} // namespace Nezumi

// -----------------------------------------------------------------------------
// Hash support for use in std containers
// -----------------------------------------------------------------------------

template<typename TObject, std::unsigned_integral TValue>
struct std::hash<Nezumi::ID<TObject, TValue>> {
    std::size_t operator()(Nezumi::ID<TObject, TValue> id) const noexcept {
        return std::hash<TValue>{}(id.value());
    }
};
