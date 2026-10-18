// =============================================================================
// EventPhase.hpp
//
// A checkpoint in the game loop at which a queue of deferred events is
// flushed. Each EventPhase owns its own queue inside EventBus.
// =============================================================================

#pragma once
#include <cstddef>
#include <cstdint>

namespace Nezumi {

enum class EventPhase : uint8_t {
    Init     = 0,
    TickPre  = 1,
    TickPost = 2,
    Quit     = 3,
    COUNT
};

inline constexpr size_t PHASE_COUNT = static_cast<size_t>(EventPhase::COUNT);

inline constexpr size_t get_phase_index(EventPhase p) noexcept {
    return static_cast<size_t>(p);
}

} // namespace Nezumi
