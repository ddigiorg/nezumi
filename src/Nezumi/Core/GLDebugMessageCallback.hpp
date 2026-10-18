// =============================================================================
// GLDebugMessageCallback.hpp
//
// Helper callback function to receive error logs, warnings, and performance
// tips directly from the graphics driver.
// =============================================================================

#pragma once
#include <glad/gl.h>
#include <iostream>

namespace Nezumi {

inline void gl_debug_message_callback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar *message,
        const void *data
) {
    const char* source_;
    const char* type_;
    const char* severity_;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        source_ = "API";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        source_ = "WINDOW SYSTEM";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        source_ = "SHADER COMPILER";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        source_ = "THIRD PARTY";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        source_ = "APPLICATION";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        source_ = "UNKNOWN";
        break;

        default:
        source_ = "UNKNOWN";
        break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        type_ = "ERROR";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        type_ = "DEPRECATED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        type_ = "UDEFINED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        type_ = "PORTABILITY";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        type_ = "PERFORMANCE";
        break;

        case GL_DEBUG_TYPE_OTHER:
        type_ = "OTHER";
        break;

        case GL_DEBUG_TYPE_MARKER:
        type_ = "MARKER";
        break;

        default:
        type_ = "UNKNOWN";
        break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        severity_ = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        severity_ = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        severity_ = "LOW";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        severity_ = "NOTIFICATION";
        break;

        default:
        severity_ = "UNKNOWN";
        break;
    }

    std::cerr << "OpenGL Error:"              << std::endl
              << "- id = "       << id        << std::endl
              << "- type = "     << type_     << std::endl
              << "- severity = " << severity_ << std::endl
              << "- source = "   << source_   << std::endl
              << "- message = "  << message   << std::endl;
};

} // namespace Nezumi
