// =============================================================================
// StoreOp.hpp
//
// Defines how the contents of an image attachment are treated at the end of a
// render pass.
//
// WebGPU: a property in render pass color and depth/stencil attachments
// Vulkan: VkAttachmentStoreOp
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class StoreOp : uint8_t {
    Store    = 0, // keep results after the pass
    DontCare = 1, // discard (e.g. depth buffer you won't read again)
};

} // namespace Nezumi::RHI
