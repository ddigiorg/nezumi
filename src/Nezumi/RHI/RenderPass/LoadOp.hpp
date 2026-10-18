// =============================================================================
// LoadOp.hpp
//
// Defines how the contents of an attachment are treated at the beginning of a
// render pass.
//
// WebGPU: GPULoadOp
// Vulkan: VkAttachmentLoadOp
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class LoadOp : uint8_t {
    Load     = 0, // keep previous contents
    Clear    = 1, // clear to the attachment's clear value
    DontCare = 2, // contents undefined; fastest when fully overwritten
};

} // namespace Nezumi::RHI
