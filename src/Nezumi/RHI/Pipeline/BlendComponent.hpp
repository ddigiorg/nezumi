// =============================================================================
// BlendComponent.hpp
//
// Defines a pipeline color blend attachment state.
//
// WebGPU: GPUBlendComponent
// Vulkan: VkPipelineColorBlendAttachmentState
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/BlendFactor.hpp"
#include "Nezumi/RHI/Pipeline/BlendOp.hpp"

namespace Nezumi::RHI {

struct BlendComponent {
    BlendFactor src = BlendFactor::One;
    BlendFactor dst = BlendFactor::Zero;
    BlendOp     op  = BlendOp::Add;
};

} // namespace Nezumi::RHI
