// =============================================================================
// ClockConfig.hpp
//
// Config of a clock.
// =============================================================================

#pragma once

namespace Nezumi {

struct ClockConfig {
    float fixed_timestep  = 1.0f / 60.0f; // seconds per fixed update (60 UPS)
    float max_frame_delta = 0.1f;         // clamp frame delta to this (spiral of death prevention)
    float max_accumulator = 0.25f;        // clamp accumulator to this (catch-up prevention)
};

} // namespace Nezumi
