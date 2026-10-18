// =============================================================================
// NullLogger
//
// Satisfies ILogger, but silently does nothing for use in tests.
// =============================================================================

#pragma once
#include "Nezumi/Logger/ILogger.hpp"
#include "Nezumi/Logger/LoggerLevel.hpp"
#include <string_view>

namespace Nezumi {

class NullLogger final : public ILogger {
public:
    void log(LoggerLevel, std::string_view) override {}
};

} // namespace Nezumi
