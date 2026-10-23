// =============================================================================
// GLInfo.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLInfo.hpp"

#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include <algorithm>
#include <cstdint>
#include <glad/gl.h>
#include <string>

namespace Nezumi::RHI::GL {

namespace {

std::string get_string(GLenum name) {
    const GLubyte* s = glGetString(name);
    return s ? std::string(reinterpret_cast<const char*>(s)) : std::string();
}

std::uint32_t get_uint(GLenum name) {
    GLint value = 0;
    glGetIntegerv(name, &value);
    return value > 0 ? static_cast<std::uint32_t>(value) : 0u;
}

} // namespace

DeviceInfo query_device_info() {
    DeviceInfo info;

    info.api_name    = "OpenGL";
    info.api_version = get_string(GL_VERSION);
    info.device_name = get_string(GL_RENDERER);

    info.max_texture_size         = get_uint(GL_MAX_TEXTURE_SIZE);
    info.max_texture_array_layers = get_uint(GL_MAX_ARRAY_TEXTURE_LAYERS);
    info.max_color_attachments    = get_uint(GL_MAX_COLOR_ATTACHMENTS);
    info.max_vertex_attributes    = get_uint(GL_MAX_VERTEX_ATTRIBS);

    // GL 3.3 binds buffers per attribute (binding slots are 4.3), so a slot
    // is the backend's own idea, and each needs at least one attribute.
    info.max_vertex_buffers = std::min(MAX_VERTEX_BUFFERS, info.max_vertex_attributes);

    GLint64 ubo_size = 0;
    glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &ubo_size);
    info.max_uniform_buffer_range = ubo_size > 0 ? static_cast<std::uint64_t>(ubo_size) : 0u;

    info.uniform_buffer_offset_alignment = get_uint(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);

    // GL has no bind groups; the backend flattens (group, binding) onto GL
    // binding points. Four groups is the RHI's own convention.
    info.max_bind_groups = 4;

    // Anisotropic filtering is core in 4.6 and an extension before that,
    // supported by practically every desktop driver.
    if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic) {
        GLfloat anisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &anisotropy);
        info.max_sampler_anisotropy = anisotropy;
    }

    info.supports_compute       = false;  // 4.3
    info.supports_base_instance = true;   // emulated (GLCommandEncoder's draws)
    info.supports_glsl_shaders  = true;
    info.supports_spirv_shaders = false;  // 4.6

    // GLContext sets glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE) when the
    // driver has it (core in 4.5, GL_ARB_clip_control before).
    info.clip_depth_zero_to_one = GLAD_GL_ARB_clip_control != 0;

    return info;
}

GLBindingLimits query_binding_limits() {
    GLBindingLimits limits;
    limits.uniform_buffers = get_uint(GL_MAX_UNIFORM_BUFFER_BINDINGS);
    limits.texture_units   = get_uint(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    return limits;
}

void log_driver_info(Logger& logger) {
    logger.info("OpenGL context created");
    logger.info("  Vendor:   {}", get_string(GL_VENDOR));
    logger.info("  Renderer: {}", get_string(GL_RENDERER));
    logger.info("  Version:  {}", get_string(GL_VERSION));
    logger.info("  GLSL:     {}", get_string(GL_SHADING_LANGUAGE_VERSION));
}

} // namespace Nezumi::RHI::GL
