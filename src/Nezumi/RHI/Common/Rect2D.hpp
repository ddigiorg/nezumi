// =============================================================================
// Rect2D.hpp
//
// A two-dimensional subregion of pixels within an image or a framebuffer.
//
// Vulkan: VkRect2D
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Extent2D.hpp"
#include "Nezumi/RHI/Common/Offset2D.hpp"

namespace Nezumi::RHI {

struct Rect2D {
    Offset2D offset;
    Extent2D extent;
};

} // namespace Nezumi::RHI
