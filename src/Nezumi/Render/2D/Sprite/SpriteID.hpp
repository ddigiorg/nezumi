// =============================================================================
// SpriteID.hpp
//
// Dense indices into a SpriteSheet's sprite and animation tables. Resolve them
// from names once (SpriteSheet::sprite / animation) and keep the ids.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t

namespace Nezumi::Render {

using SpriteID    = std::uint32_t;
using AnimationID = std::uint32_t;

} // namespace Nezumi::Render
