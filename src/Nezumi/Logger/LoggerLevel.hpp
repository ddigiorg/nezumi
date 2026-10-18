// =============================================================================
// LoggerLevel.hpp
//
// Severity levels used throughout the logging system, plus a helper to
// convert a level to its fixed-width string representation.
// =============================================================================

#pragma once
#include <string_view>

namespace Nezumi {

enum class LoggerLevel {
    Trace,   // Very detailed, usually only for debugging
    Debug,   // Detailed information for diagnosing problems
    Info,    // General information about program execution
    Warn,    // Warning messages for potential issues
    Error,   // Error messages for failures
    Critical // Critical errors that might crash the program
};

inline std::string_view to_string(LoggerLevel level) {
    switch (level) {
        case LoggerLevel::Trace:    return "TRACE";
        case LoggerLevel::Debug:    return "DEBUG";
        case LoggerLevel::Info:     return "INFO ";
        case LoggerLevel::Warn:     return "WARN ";
        case LoggerLevel::Error:    return "ERROR";
        case LoggerLevel::Critical: return "CRIT ";
    }
    return "?????";
}

} // namespace Nezumi
