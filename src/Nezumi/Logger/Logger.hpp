// =============================================================================
// Logger.hpp
//
// The real logger implementation that writes to console and/or file.
// =============================================================================

#pragma once
#include "Nezumi/Logger/ILogger.hpp"
#include "Nezumi/Logger/LoggerConfig.hpp"
#include "Nezumi/Logger/LoggerLevel.hpp"
#include <fstream>
#include <string>
#include <string_view>

namespace Nezumi {

class Logger final : public ILogger {
public:
    Logger(const LoggerConfig& config = {});
    ~Logger() override;

    void log(LoggerLevel level, std::string_view message) override;

private:
    std::string make_prefix_(LoggerLevel level) const;

    LoggerConfig  config_;
    std::ofstream log_file_;
};

} // namespace Nezumi
