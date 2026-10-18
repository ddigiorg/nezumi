// =============================================================================
// Extent2D.hpp
//
// Width and height in pixels. Used for backbuffer size and resizing.
//
// Vulkan: VkExtent2D
// =============================================================================

#pragma once
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

struct Extent2D {
    std::uint32_t width  = 0;
    std::uint32_t height = 0;

    constexpr bool operator==(const Extent2D&) const noexcept = default;
};

} // namespace Nezumi::RHI
