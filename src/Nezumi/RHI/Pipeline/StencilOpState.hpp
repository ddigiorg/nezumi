// =============================================================================
// StencilOpState.hpp
//
// Defines stencil operation state.
//
// WebGPU: GPUStencilFaceState
// Vulkan: VkStencilOpState
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/CompareOp.hpp"
#include "Nezumi/RHI/Pipeline/StencilOp.hpp"

namespace Nezumi::RHI {

struct StencilOpState {
    CompareOp compare    = CompareOp::Always;
    StencilOp fail       = StencilOp::Keep;
    StencilOp depth_fail = StencilOp::Keep;
    StencilOp pass       = StencilOp::Keep;
};

} // namespace Nezumi::RHI
