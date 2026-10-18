// =============================================================================
// ColorWriteFlags.hpp
//
// Defines which color components (Red, Green, Blue, Alpha) are written to a
// framebuffer attachment.
//
// WebGPU: GPUColorWrite
// Vulkan: VkColorComponentFlags
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class ColorWriteFlags : uint8_t {
    None  = 0,
    Red   = 1u << 0,
    Green = 1u << 1,
    Blue  = 1u << 2,
    Alpha = 1u << 3,
    All   = Red | Green | Blue | Alpha,
};
ENUM_CLASS_FLAGS(ColorWriteFlags);

} // namespace Nezumi::RHI
