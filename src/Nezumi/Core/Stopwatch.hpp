// =============================================================================
// Stopwatch.hpp
//
// Lightweight stopwatch for timing code sections.
// =============================================================================

#pragma once
#include <chrono>

namespace Nezumi {

class Stopwatch {
public:
    Stopwatch() { start(); }

    void start() { start_ = clock_::now(); }

    float get_s()  const { return duration_<std::ratio<1>>(); }
    float get_ms() const { return duration_<std::milli>();    }
    float get_us() const { return duration_<std::micro>();    }

private:
    using clock_ = std::chrono::steady_clock;
    using point_ = clock_::time_point;

    template<typename Ratio>
    float duration_() const {
        return std::chrono::duration<float, Ratio>(clock_::now() - start_).count();
    }

    point_ start_;
};

} // namespace Nezumi
