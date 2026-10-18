// =============================================================================
// Clock.cpp
// =============================================================================

#include "Nezumi/Core/Clock.hpp"
#include <algorithm> // std::min
#include <thread>    // std::this_thread

namespace Nezumi {

Clock::Clock(const ClockConfig& config)
    : FIXED_TIMESTEP_ (config.fixed_timestep )
    , MAX_FRAME_DELTA_(config.max_frame_delta)
    , MAX_ACCUMULATOR_(config.max_accumulator)
    , time_curr_(now_())
    , time_prev_(now_())
{}

void Clock::reset() {
    time_prev_   = now_();
    frame_delta_ = 0.0f;
    accumulator_ = 0.0f;
}

void Clock::begin(bool advance_sim) {

    // Measure real elapsed time since last iteration
    time_curr_ = now_();
    elapsed_   = to_seconds_(time_curr_ - time_prev_);
    time_prev_ = time_curr_;

    // Clamp delta
    // Prevents spiral of death if an iteration takes too long
    frame_delta_ = std::min(elapsed_, MAX_FRAME_DELTA_);

    // Clamp accumulator
    // Prevents catch-up death if game falls behind more than MAX_ACCUMULATOR_
    // seconds worth of ticks.  When simulation is paused the accumulator is
    // left untouched.
    if (advance_sim)
        accumulator_ = std::min(accumulator_ + frame_delta_, MAX_ACCUMULATOR_);
}

bool Clock::should_update() {
    if (accumulator_ < FIXED_TIMESTEP_)
        return false;

    accumulator_  -= FIXED_TIMESTEP_;
    current_tick_ += 1;

    return true;
}

void Clock::wait_until_next_tick() {
    const float remaining = FIXED_TIMESTEP_ - accumulator_;
    if (remaining <= 0.0f) return; // A tick is already due, don't sleep

    using namespace std::chrono;
    const auto deadline = time_curr_ + duration_cast<ChronoClock_::duration>(
                                       duration<float>(remaining));
    std::this_thread::sleep_until(deadline);
}

} // namespace Nezumi
