// =============================================================================
// InputSource.hpp
//
// A physical input an action can be bound to: a keyboard key (by scancode, so
// bindings follow key position, not layout) or a mouse button.
// =============================================================================

#pragma once
#include "Nezumi/Input/Mouse.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include <cstdint> // uint8_t, uint32_t

namespace Nezumi {

struct InputSource {
    enum class Kind : std::uint8_t {
        Key,
        MouseButton,
    };

    Kind          kind = Kind::Key;
    std::uint32_t code = 0u; // ScanCode or MouseButton value

    static constexpr InputSource key(ScanCode scancode) {
        return { Kind::Key, static_cast<std::uint32_t>(scancode) };
    }

    static constexpr InputSource mouse(MouseButton button) {
        return { Kind::MouseButton, static_cast<std::uint32_t>(button) };
    }

    constexpr bool operator==(const InputSource&) const = default;
};

} // namespace Nezumi
