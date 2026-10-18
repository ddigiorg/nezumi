// =============================================================================
// Logger.cpp
// =============================================================================

#include "Nezumi/Logger/Logger.hpp"
#include <chrono>
#include <iostream>

namespace Nezumi {

Logger::Logger(const LoggerConfig& config)
    : config_(config)
{
    if (config_.log_to_file) {
        log_file_.open(config_.filepath, std::ios::out | std::ios::app);

        // File failed: fall back to console only, log the problem
        if (!log_file_.is_open()) {
            config_.log_to_file = false;
            error("Failed to open log file: {}", config_.filepath);
        }
    }
}

Logger::~Logger() {
    if (log_file_.is_open()) log_file_.close();
}

void Logger::log(LoggerLevel level, std::string_view message) {
    if (level < config_.min_level)
        return;

    const std::string line = make_prefix_(level) + std::string(message);

    if (config_.log_to_console) {
        auto& stream = (level >= LoggerLevel::Error) ? std::cerr : std::cout;
        stream << line << "\n";
    }

    if (config_.log_to_file && log_file_.is_open()) {
        log_file_ << line << "\n";
        log_file_.flush();
    }
}

std::string Logger::make_prefix_(LoggerLevel level) const {
    std::string prefix;

    if (config_.include_time) {
        const auto now   = std::chrono::system_clock::now();
        const auto ttime = std::chrono::system_clock::to_time_t(now);

        std::tm tm_buf{};
        #ifdef _WIN32
            localtime_s(&tm_buf, &ttime);
        #else
            localtime_r(&ttime, &tm_buf);
        #endif

        prefix += std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d} ",
            tm_buf.tm_year + 1900,
            tm_buf.tm_mon  + 1,
            tm_buf.tm_mday,
            tm_buf.tm_hour,
            tm_buf.tm_min,
            tm_buf.tm_sec);
    }

    if (config_.include_level)
        prefix += std::format("[{}] ", to_string(level));

    return prefix;
}

} // namespace Nezumi
