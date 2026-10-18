// =============================================================================
// DepthStencilState.hpp
//
// Defines depth testing, depth writing, and stencil operations.
//
// Format::Undefined means the pipeline renders without a depth attachment.
// The stencil reference value is dynamic state, set on the encoder.
//
// WebGPU: GPUDepthStencilState
// Vulkan: VkPipelineDepthStencilStateCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/CompareOp.hpp"
#include "Nezumi/RHI/Pipeline/StencilOpState.hpp"
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

struct DepthStencilState {
    Format         format             = Format::Undefined;
    bool           depth_test         = false;
    bool           depth_write        = false;
    CompareOp      depth_compare      = CompareOp::Always;
    bool           stencil_test       = false;
    StencilOpState stencil_front      = {};
    StencilOpState stencil_back       = {};
    std::uint8_t   stencil_read_mask  = 0xFF;
    std::uint8_t   stencil_write_mask = 0xFF;
};

} // namespace Nezumi::RHI
