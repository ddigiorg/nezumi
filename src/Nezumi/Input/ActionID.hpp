// =============================================================================
// ActionID.hpp
//
// Input action identification. An action is something the player does
// ("move up", "attack", "pan camera left"), bound to one or more keys or mouse
// buttons in an InputMap.
//
// The engine names no actions: the game declares its own, as small dense
// values (they index the InputMap's state array):
//
//   namespace Action {
//       inline constexpr ActionID MoveLeft  = 0;
//       inline constexpr ActionID MoveRight = 1;
//       ...
//   }
// =============================================================================

#pragma once
#include "Nezumi/Object/ID.hpp"
#include <cstdint> // uint16_t

namespace Nezumi {

struct ActionTag;

using ActionID = ID<ActionTag, std::uint16_t>;

} // namespace Nezumi
