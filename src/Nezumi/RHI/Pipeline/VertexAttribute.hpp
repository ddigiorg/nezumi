// =============================================================================
// VertexAttribute.hpp
//
// A specific piece of data (e.g. position, color, normal, texture coordinate)
// associated with an individual vertex and passed into the vertex shader.
//
// WebGPU: GPUVertexAttribute
// Vulkan: VkVertexInputAttributeDescription
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/VertexFormat.hpp"
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

struct VertexAttribute {
    std::uint32_t location = 0;                     // layout(location = N)
    VertexFormat  format   = VertexFormat::Float32;
    std::uint32_t offset   = 0;                     // bytes from start of element
};

} // namespace Nezumi::RHI
