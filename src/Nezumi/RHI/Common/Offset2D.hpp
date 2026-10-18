// =============================================================================
// Offset2D.hpp
//
// A 2D pixel coordinate or position within a rectangular region, such as a
// frame, image, or window.
//
// Vulkan: VkOffset2D
// =============================================================================

#pragma once
#include <cstdint> // int32_t

namespace Nezumi::RHI {

struct Offset2D {
    std::int32_t x = 0;
    std::int32_t y = 0;

    constexpr bool operator==(const Offset2D&) const noexcept = default;
};

} // namespace Nezumi::RHI
