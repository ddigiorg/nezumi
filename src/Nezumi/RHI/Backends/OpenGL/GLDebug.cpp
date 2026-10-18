// =============================================================================
// GLDebug.cpp
// =============================================================================
#include "Nezumi/RHI/Backends/OpenGL/GLDebug.hpp"

#include <glad/gl.h>

#include "Nezumi/Logger/Logger.hpp"

namespace Nezumi::RHI::GL {

namespace {

const char* debug_source_name(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API:             return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
        default:                              return "Other";
    }
}

const char* debug_type_name(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
        case GL_DEBUG_TYPE_MARKER:              return "Marker";
        default:                                return "Other";
    }
}

// `user` is the Logger*, passed through glDebugMessageCallback. GLAD_API_PTR
// is the calling convention the driver expects (it matters on Windows).
void GLAD_API_PTR debug_message_callback(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       /*length*/,
    const GLchar* message,
    const void*   user)
{
    auto* logger = static_cast<Logger*>(const_cast<void*>(user));

    const char* src = debug_source_name(source);
    const char* typ = debug_type_name(type);

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            logger->error("OpenGL [{} / {} / {}]: {}", src, typ, id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            logger->warn("OpenGL [{} / {} / {}]: {}", src, typ, id, message);
            break;
        default:
            logger->info("OpenGL [{} / {} / {}]: {}", src, typ, id, message);
            break;
    }
}

} // namespace

void enable_debug_output(Logger& logger) {
    glEnable(GL_DEBUG_OUTPUT);

    // Report messages on the thread and call stack that caused them, so a
    // breakpoint in the callback lands on the offending GL call. Costs
    // performance; debug builds only.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageCallback(debug_message_callback, &logger);

    // Drop notification-level chatter ("buffer will use video memory", ...).
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                          GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // Shader compile and link errors are reported by GLDevice with the
    // shader's name attached; the driver's copy would just duplicate them.
    glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DONT_CARE,
                          GL_DONT_CARE, 0, nullptr, GL_FALSE);
}

} // namespace Nezumi::RHI::GL
