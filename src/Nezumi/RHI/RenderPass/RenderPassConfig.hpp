// =============================================================================
// RenderPassConfig.hpp
//
// A config of a RenderPass.
//
// WebGPU: GPURenderPassDescriptor
// =============================================================================

#pragma once
#include "Nezumi/RHI/RenderPass/ColorAttachment.hpp"
#include "Nezumi/RHI/RenderPass/DepthStencilAttachment.hpp"
#include <span>        // uint8_t
#include <string_view> // string_view

namespace Nezumi::RHI {

struct RenderPassConfig {
    std::string_view                 name              = "";
    std::span<const ColorAttachment> color_attachments = {};
    DepthStencilAttachment           depth_stencil     = {};
};

} // namespace Nezumi::RHI
