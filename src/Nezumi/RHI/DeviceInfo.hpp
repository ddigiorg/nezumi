// =============================================================================
// DeviceInfo.hpp
//
// What the device can do, queried once at creation. Code above the RHI
// branches on these instead of on which backend is active.
//
// Unlike configs, this is owned data: the Device holds it and hands out a
// const reference, so the strings are std::string.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t
#include <string>  // string

namespace Nezumi::RHI {

struct DeviceInfo {

    // -------------------------------------------------------------------------
    // Identification (for logs and bug reports)
    // -------------------------------------------------------------------------

    std::string api_name;    // "OpenGL"
    std::string api_version; // "4.6.0 NVIDIA 555.85"
    std::string device_name; // GPU / renderer string

    // -------------------------------------------------------------------------
    // Limits
    // -------------------------------------------------------------------------

    std::uint32_t max_texture_size         = 0; // max width/height of a 2D texture
    std::uint32_t max_texture_array_layers = 0;
    std::uint32_t max_color_attachments    = 0;
    std::uint32_t max_bind_groups          = 0;
    std::uint32_t max_vertex_buffers       = 0;
    std::uint32_t max_vertex_attributes    = 0;
    std::uint64_t max_uniform_buffer_range = 0; // bytes visible through one binding

    // Buffer offsets in bind groups, and dynamic offsets, must be multiples of
    // this. Commonly 256. The per-frame ring allocator rounds up to it.
    std::uint32_t uniform_buffer_offset_alignment = 256;

    float max_sampler_anisotropy = 1.0f;

    // -------------------------------------------------------------------------
    // Features
    // -------------------------------------------------------------------------

    bool supports_compute       = false;
    bool supports_base_instance = false;   // first_instance != 0 in draw calls
    bool supports_spirv_shaders = false;
    bool supports_glsl_shaders  = false;

    // -------------------------------------------------------------------------
    // Conventions
    //
    // The RHI's clip-space depth range is 0..1 (Vulkan / D3D). The GL backend
    // conforms using glClipControl where available; if it can't, this is
    // false and depth is GL's native -1..1. Projection matrices check it.
    // -------------------------------------------------------------------------

    bool clip_depth_zero_to_one = true;
};

} // namespace Nezumi::RHI
