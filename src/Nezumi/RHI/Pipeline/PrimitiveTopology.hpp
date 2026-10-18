// =============================================================================
// PrimitiveTopology.hpp
//
// Defines how the Input Assembler organizes a sequence of vertices into
// geometric shapes like points, lines, or triangles.
//
// WebGPU: A setting inside the render pipeline's primitive configuration
// Vulkan: VkPrimitiveTopology
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class PrimitiveTopology : uint8_t {
    TriangleList  = 0,
    TriangleStrip = 1,
    LineList      = 2,
    LineStrip     = 3,
    PointList     = 4,
};

} // namespace Nezumi::RHI
