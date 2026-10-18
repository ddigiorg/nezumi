// =============================================================================
// VertexBufferLayout.hpp
//
// Defines the byte stride between consecutive vertices, and whether the data is
// fetched per-vertex or per-instance
//
// The index of a layout in PipelineConfig::vertex_buffers is the slot passed to
// CommandEncoder::set_vertex_buffer.
//
// WebGPU: GPUVertexBufferLayout
// Vulkan: VkVertexInputBindingDescription
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/VertexAttribute.hpp"
#include "Nezumi/RHI/Pipeline/VertexStepMode.hpp"
#include <cstdint> // uint32_t
#include <span>    // span

namespace Nezumi::RHI {

struct VertexBufferLayout {
    std::uint32_t                    stride     = 0;
    VertexStepMode                   step_mode  = VertexStepMode::Vertex;
    std::span<const VertexAttribute> attributes = {};
};

} // namespace Nezumi::RHI
