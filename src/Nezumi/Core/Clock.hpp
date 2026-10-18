// =============================================================================
// Clock.hpp
//
// Manages fixed-timestep game loop timing.
//
// The game simulation has no knowledge of wall clock time. Each tick always
// advances the game world by exactly fixed_step seconds of game time,
// regardless of how long that tick took to execute in real time.
// =============================================================================

#pragma once
#include "Nezumi/Core/ClockConfig.hpp"
#include "Nezumi/Core/Tick.hpp"
#include <chrono>

namespace Nezumi {

class Clock {
public:
    Clock(const ClockConfig& config = {});

    void reset();
    void begin(bool advance_sim = true);
    bool should_update();
    void wait_until_next_tick();

    Tick  get_tick()     const { return current_tick_;   }
    float get_fixed_dt() const { return FIXED_TIMESTEP_; }
    float get_real_dt()  const { return elapsed_;        }

private:
    using ChronoClock_     = std::chrono::steady_clock;
    using ChronoTimePoint_ = ChronoClock_::time_point;

    static ChronoTimePoint_ now_() { return ChronoClock_::now(); }

    static float to_seconds_(ChronoClock_::duration d) {
        return std::chrono::duration<float>(d).count();
    }

    const float FIXED_TIMESTEP_  = 0.0f; // seconds
    const float MAX_FRAME_DELTA_ = 0.0f; // seconds
    const float MAX_ACCUMULATOR_ = 0.0f; // seconds

    ChronoTimePoint_ time_curr_  = {};
    ChronoTimePoint_ time_prev_  = {};

    float elapsed_      = 0.0f; // seconds
    float frame_delta_  = 0.0f; // seconds
    float accumulator_  = 0.0f; // seconds
    Tick  current_tick_ = 0u;
};

} // namespace Nezumi
