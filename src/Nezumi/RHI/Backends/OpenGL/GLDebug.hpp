// =============================================================================
// GLDebug.hpp
//
// Routes OpenGL driver messages (errors, warnings, perf hints) to the logger.
// =============================================================================

#pragma once
class Logger;

namespace Nezumi::RHI::GL {

// Enables synchronous debug output and installs a callback that forwards
// driver messages to `logger`, mapping GL severity to log level. Notification-
// level messages are filtered out.
//
// Requires a current context created with the debug flag, and GL_KHR_debug
// (core in 4.3; check GLAD_GL_KHR_debug first on older drivers). The logger
// must outlive the context, since the driver keeps a pointer to it.
void enable_debug_output(Logger& logger);

} // namespace Nezumi::RHI::GL
