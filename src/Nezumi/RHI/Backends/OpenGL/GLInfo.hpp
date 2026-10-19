// =============================================================================
// GLInfo.hpp
//
// Queries the driver for its identity, limits, and features.
// Both functions require a current OpenGL context.
// =============================================================================

#pragma once
#include "Nezumi/RHI/DeviceInfo.hpp"
#include <cstdint>
class Logger;

namespace Nezumi::RHI::GL {

// Driver strings, limits, and features, as RHI::DeviceInfo. Queries the
// driver each call; call once and keep the result.
DeviceInfo query_device_info();

// Binding-point limits the backend checks bind groups against. Not part of
// DeviceInfo because they're specific to how GL flattens bind groups.
// Storage buffers and image units (GL 4.3 / 4.2) aren't available on 3.3.
struct GLBindingLimits {
    std::uint32_t uniform_buffers = 0;   // GL_MAX_UNIFORM_BUFFER_BINDINGS
    std::uint32_t texture_units   = 0;   // GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
};

GLBindingLimits query_binding_limits();

// Logs vendor, renderer, and version strings at info level.
void log_driver_info(Logger& logger);

} // namespace Nezumi::RHI::GL
