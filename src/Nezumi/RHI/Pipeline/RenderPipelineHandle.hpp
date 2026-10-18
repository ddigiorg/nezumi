// =============================================================================
// RenderPipelineHandle.hpp
//
// A handle to a monolithic GPU state object that defines how data is processed
// to render graphics or run compute tasks.
//
// WebGPU: GPURenderPipeline
// Vulkan: VkPipeline
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct RenderPipelineTag;
using  RenderPipelineHandle = Handle<RenderPipelineTag>;

} // namespace Nezumi::RHI
