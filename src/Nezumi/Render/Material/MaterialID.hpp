// =============================================================================
// MaterialID.hpp
//
// Material identification. Index into the renderer's material table.
// =============================================================================

#pragma once
#include <cstdint> // uint16_t

namespace Nezumi::Render {

using MaterialID = std::uint16_t;

struct Material {
    static constexpr MaterialID None = 0;  // default
};

} // namespace Nezumi::Render
