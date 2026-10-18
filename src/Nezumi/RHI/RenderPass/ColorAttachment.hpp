// =============================================================================
// ColorAttachment.hpp
//
// WebGPU: GPURenderPassColorAttachment
// Vulkan: VkAttachmentDescription
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Color.hpp"
#include "Nezumi/RHI/RenderPass/LoadOp.hpp"
#include "Nezumi/RHI/RenderPass/StoreOp.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"

namespace Nezumi::RHI {

struct ColorAttachment {
    TextureHandle texture     = {};
    LoadOp        load_op     = LoadOp::Clear;
    StoreOp       store_op    = StoreOp::Store;
    Color         clear_color = {};
};

} // namespace Nezumi::RHI
