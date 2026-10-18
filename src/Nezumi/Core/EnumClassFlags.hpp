// =============================================================================
// EnumClassFlags.hpp
//
// Macro that automatically overloads standard bitwise operators for a scoped
// enumeration (enum class), allowing you to use it safely as a bitmask/bitflag.
//
// Example namespace-scope enum:
//
//   enum class Flags : std::uint8_t {
//     None  = 0,
//     Flag0 = 1 << 0,
//     Flag1 = 1 << 1
//   };
//   ENUM_CLASS_FLAGS(Flags);
//
// Example enum nested inside a class or struct:
//
//   struct Entity {
//     enum class State : std::uint8_t {
//       None  = 0,
//       Alive = 1 << 0,
//     }
//     ENUM_CLASS_FLAGS_NESTED(State);
//   }
//
// Generates:
//
//   |, &, ^, ~, |=, &=, ^=,
//   has_any(value, mask), has_all(value, mask), has_none(value, mask)
//
// Rules:
//
// - Invoke ENABLE_ENUM_CLASS_FLAGS in the same namespace as the enum so
//   argument-dependent lookup finds the operators.
// - Give the enum a fixed underlying type (enum class X : std::uint8_t).
//   Plain `enum class X` defaults to int, which works but wastes space and
//   makes ~ produce negative values.
// =============================================================================

#pragma once
#include <type_traits> // is_enum_v, is_convertible_v, underlying_type_t

namespace Nezumi {

// Shared body. PREFIX is empty at namespace scope or `friend` inside a class.
#define ENUM_CLASS_FLAGS_IMPL_(E, PREFIX)                                      \
    /* Bitwise AND */                                                          \
    PREFIX constexpr E operator&(E a, E b) noexcept {                          \
        using U = std::underlying_type_t<E>;                                   \
        return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));          \
    }                                                                          \
    /* Bitwise OR */                                                           \
    PREFIX constexpr E operator|(E a, E b) noexcept {                          \
        using U = std::underlying_type_t<E>;                                   \
        return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));          \
    }                                                                          \
    /* Bitwise XOR */                                                          \
    PREFIX constexpr E operator^(E a, E b) noexcept {                          \
        using U = std::underlying_type_t<E>;                                   \
        return static_cast<E>(static_cast<U>(a) ^ static_cast<U>(b));          \
    }                                                                          \
    /* Bitwise NOT */                                                          \
    PREFIX constexpr E operator~(E a) noexcept {                               \
        using U = std::underlying_type_t<E>;                                   \
        return static_cast<E>(static_cast<U>(~static_cast<U>(a)));             \
    }                                                                          \
    /* Bitwise AND assignment */                                               \
    PREFIX constexpr E& operator&=(E& a, E b) noexcept { return a = a & b; }   \
    /* Bitwise OR  assignment */                                               \
    PREFIX constexpr E& operator|=(E& a, E b) noexcept { return a = a | b; }   \
    /* Bitwise XOR assignment */                                               \
    PREFIX constexpr E& operator^=(E& a, E b) noexcept { return a = a ^ b; }   \
    /* True if any bit in mask is set in value. */                             \
    PREFIX constexpr bool has_any(E value, E mask) noexcept {                  \
        return (value & mask) != E{};                                          \
    }                                                                          \
    /* True if every bit in mask is set in value. */                           \
    PREFIX constexpr bool has_all(E value, E mask) noexcept {                  \
        return (value & mask) == mask;                                         \
    }                                                                          \
    /* True if no bit in mask is set in value. */                              \
    PREFIX constexpr bool has_none(E value, E mask) noexcept {                 \
        return (value & mask) == E{};                                          \
    }                                                                          \
    static_assert(std::is_enum_v<E>, #E " is not an enum");                    \
    static_assert(!std::is_convertible_v<E, std::underlying_type_t<E>>,        \
                  #E " must be an enum class, not a plain enum")

// Use at namespace scope, in the enum's own namespace.
#define ENUM_CLASS_FLAGS(E) ENUM_CLASS_FLAGS_IMPL_(E, )

// Use inside the class or struct that declares the enum. The operators become
// hidden friends, found through argument-dependent lookup.
#define ENUM_CLASS_FLAGS_NESTED(E) ENUM_CLASS_FLAGS_IMPL_(E, friend)

} // namespace Nezumi
