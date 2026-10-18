// =============================================================================
// LoggerConfig.hpp
//
// Config of a logger.
// =============================================================================

#pragma once
#include "Nezumi/Logger/LoggerLevel.hpp"

namespace Nezumi {

struct LoggerConfig {
    const char* filepath       = "game.log";
    LoggerLevel min_level      = LoggerLevel::Info;
    bool        log_to_console = true;
    bool        log_to_file    = false;
    bool        include_time   = true;
    bool        include_level  = true;
};

} // namespace Nezumi
