// =============================================================================
// DrawCommand.hpp
//
// One submitted item: where it sorts and where its instance bytes live in
// the CommandQueue's arena. 16 bytes, so sorting moves very little memory.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t, uint64_t

namespace Nezumi::Render {

struct DrawCommand {
    std::uint64_t key    = 0; // SortKey
    std::uint32_t offset = 0; // byte offset into the arena
    std::uint32_t size   = 0; // instance size in bytes (== the kind's stride)
};

static_assert(sizeof(DrawCommand) == 16);

} // namespace Nezumi::Render
