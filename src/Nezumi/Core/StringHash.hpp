// =============================================================================
// StringHash
//
// Enables lookup by string_view without allocating.
// =============================================================================

#pragma once
#include <cstddef>     // size_t
#include <functional>  // hash
#include <string_view>

namespace Nezumi {

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

} // namespace Nezumi
