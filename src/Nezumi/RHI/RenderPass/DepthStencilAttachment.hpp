// =============================================================================
// DepthStencilAttachment.hpp
//
// texture null = no depth/stencil attachment for this pass.
//
// WebGPU: A property in GPURenderPassDescriptor
// Vulkan: VkAttachmentDescription
// =============================================================================

#pragma once
#include "Nezumi/RHI/RenderPass/LoadOp.hpp"
#include "Nezumi/RHI/RenderPass/StoreOp.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

struct DepthStencilAttachment {
    TextureHandle texture          = {};
    LoadOp        depth_load_op    = LoadOp::Clear;
    StoreOp       depth_store_op   = StoreOp::DontCare;
    float         clear_depth      = 1.0f;
    LoadOp        stencil_load_op  = LoadOp::Clear;
    StoreOp       stencil_store_op = StoreOp::DontCare;
    std::uint8_t  clear_stencil    = 0;
};

} // namespace Nezumi::RHI
