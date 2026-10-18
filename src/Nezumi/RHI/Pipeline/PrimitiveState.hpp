// =============================================================================
// PrimitiveState.hpp
//
// Defines how geometric primitives are assembled and rasterized.
//
// WebGPU: GPUPrimitiveState
// Vulkan: VkPipelineInputAssemblyStateCreateInfo and
//         VkPipelineRasterizationStateCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Pipeline/CullMode.hpp"
#include "Nezumi/RHI/Pipeline/FrontFace.hpp"
#include "Nezumi/RHI/Pipeline/PrimitiveTopology.hpp"

namespace Nezumi::RHI {

struct PrimitiveState {
    PrimitiveTopology topology   = PrimitiveTopology::TriangleList;
    CullMode          cull_mode  = CullMode::None;
    FrontFace         front_face = FrontFace::CounterClockwise;
};

} // namespace Nezumi::RHI
