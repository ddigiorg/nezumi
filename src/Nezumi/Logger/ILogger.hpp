// =============================================================================
// ILogger.hpp
//
// Interface for every logging related subsystem.
// =============================================================================

#pragma once
#include "Nezumi/Logger/LoggerLevel.hpp"
#include <format>
#include <string_view>

namespace Nezumi {

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LoggerLevel level, std::string_view message) = 0;

    // -------------------------------------------------------------------------
    // Plain string overloads
    // -------------------------------------------------------------------------

    void trace   (std::string_view msg) { log(LoggerLevel::Trace,    msg); }
    void debug   (std::string_view msg) { log(LoggerLevel::Debug,    msg); }
    void info    (std::string_view msg) { log(LoggerLevel::Info,     msg); }
    void warn    (std::string_view msg) { log(LoggerLevel::Warn,     msg); }
    void error   (std::string_view msg) { log(LoggerLevel::Error,    msg); }
    void critical(std::string_view msg) { log(LoggerLevel::Critical, msg); }

    // -------------------------------------------------------------------------
    // Formatted string overloads
    // -------------------------------------------------------------------------

    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Trace, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void critical(std::format_string<Args...> fmt, Args&&... args) {
        log(LoggerLevel::Critical, std::format(fmt, std::forward<Args>(args)...));
    }
};

} // namespace Nezumi
