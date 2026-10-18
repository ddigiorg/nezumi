// =============================================================================
// BufferHandle.hpp
//
// A handle to a linear region of device memory used to store raw, unstructured
// blocks of arbitrary data that can be accessed by the GPU.
//
// WebGPU: GPUBuffer
// Vulkan: VkBuffer
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct BufferTag;
using  BufferHandle = Handle<BufferTag>;

} // namespace Nezumi::RHI
