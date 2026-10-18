// =============================================================================
// DeviceConfig.hpp
//
// Config of a Device.  Passed to create_device().
//
// WebGPU: GPUDeviceDescriptor
// Vulkan: VkDeviceCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/BackendType.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view

namespace Nezumi::RHI {

struct DeviceConfig {
    std::string_view name = "";

    BackendType backend = BackendType::OpenGL;

    bool enable_vsync = true;

    // Enables API validation / debug output (GL_KHR_debug, Vulkan validation
    // layers) and extra RHI-level checks. Slow; on in debug builds.
    bool enable_debug = false;

    // How many frames the CPU may run ahead of the GPU. Also how long
    // destroyed resources are kept alive and how many copies per-frame
    // buffers need. 2 is the usual choice; 3 trades latency for smoothness.
    std::uint32_t frames_in_flight = 2;

    // If true, the backbuffer is an sRGB format and blending happens in
    // linear space. Your sprite textures should then use *Srgb formats too.
    // Changing this visibly alters alpha blending, so it's off by default.
    bool srgb_backbuffer = false;
};

} // namespace Nezumi::RHI
