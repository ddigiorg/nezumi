// =============================================================================
// ColorTargetState.hpp
//
// Defines how a fragment shader's color outputs interact with a render pass's
// color attachments.
//
// WebGPU: GPUColorTargetState
// Vulkan: VkPipelineColorBlendAttachmentState
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/BlendState.hpp"
#include "Nezumi/RHI/Pipeline/ColorWriteFlags.hpp"

namespace Nezumi::RHI {

struct ColorTargetState {
    Format          format     = Format::Undefined;
    BlendState      blend      = {};
    ColorWriteFlags write_mask = ColorWriteFlags::All;
};

} // namespace Nezumi::RHI
