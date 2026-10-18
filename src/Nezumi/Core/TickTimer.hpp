// =============================================================================
// TickTimer.hpp
//
// Lightweight timer for simulation ticks.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include <concepts> // std::unsigned_integral

namespace Nezumi {

template<std::unsigned_integral T = Tick>
class TickTimer {
public:
    T current_tick = 0u;
    T target_tick  = 0u;

    void restart() { current_tick = 0u; }

    bool tick() {
        current_tick++;
        if (current_tick >= target_tick) {
            current_tick = 0u;
            return true;
        }
        return false;
    }
};

} // namespace Nezumi
