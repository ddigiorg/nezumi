// =============================================================================
// Item/ItemID.hpp
//
// What an item is (wood, stone, a sword, ...): the key inventories and
// storage will count items by. The engine only defines the type; the game
// assigns the ids, in one place, so two items can never share one:
//
//   namespace Items {
//       inline constexpr Nezumi::ItemID Wood{ 0 };
//       inline constexpr Nezumi::ItemID Gold{ 1 };
//   }
//
// A null ItemID means "no item" (e.g. a resource that yields nothing).
// =============================================================================

#pragma once
#include "Nezumi/Object/ID.hpp"
#include <cstdint> // uint16_t

namespace Nezumi {

struct ItemTag;

using ItemID = ID<ItemTag, std::uint16_t>;

} // namespace Nezumi
