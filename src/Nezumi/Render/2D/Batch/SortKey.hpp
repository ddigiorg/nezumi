// =============================================================================
// SortKey.hpp
//
// Every ordering rule in one 64-bit integer. Sorting draws by this key gives
// painter's order first, and groups equal GPU state second.
//
//   bit 63                                                     bit 0
//   +--------+---------+----------+--------+-------------+---------+
//   | view 4 | layer 8 | order 24 | kind 8 | material 16 | spare 4 |
//   +--------+---------+----------+--------+-------------+---------+
//
// - view:     world or screen space, each with its own camera
// - layer:    DrawParams::layer
// - order:    DrawParams::order, biased so negative values sort first
// - kind:     KindID (which pipeline / instance layout)
// - material: MaterialID (which texture or atlas page)
//
// view/kind/material together are the "state" part of the key: two items that
// end up adjacent after sorting and share it can go in the same draw.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Kind/KindID.hpp"
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/Render/2D/DrawParams.hpp"
#include <algorithm> // clamp
#include <cstdint>   // uint32_t, uint64_t

namespace Nezumi::Render {

namespace detail {
    constexpr std::uint64_t bit_mask(std::uint32_t bits, std::uint32_t shift) {
        return ((std::uint64_t(1) << bits) - 1) << shift;
    }
} // namespace detail

struct SortKey {

    // -------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------

    static constexpr std::uint32_t SPARE_BITS    = 4;
    static constexpr std::uint32_t MATERIAL_BITS = 16;
    static constexpr std::uint32_t KIND_BITS     = 8;
    static constexpr std::uint32_t ORDER_BITS    = 24;
    static constexpr std::uint32_t LAYER_BITS    = 8;
    static constexpr std::uint32_t VIEW_BITS     = 4;

    static constexpr std::uint32_t MATERIAL_SHIFT = SPARE_BITS;
    static constexpr std::uint32_t KIND_SHIFT     = MATERIAL_SHIFT + MATERIAL_BITS;
    static constexpr std::uint32_t ORDER_SHIFT    = KIND_SHIFT     + KIND_BITS;
    static constexpr std::uint32_t LAYER_SHIFT    = ORDER_SHIFT    + ORDER_BITS;
    static constexpr std::uint32_t VIEW_SHIFT     = LAYER_SHIFT    + LAYER_BITS;

    static_assert(VIEW_SHIFT + VIEW_BITS == 64, "SortKey must fill 64 bits");

    // Bits that must match for two adjacent items to share a draw.
    static constexpr std::uint64_t STATE_MASK =
        detail::bit_mask(VIEW_BITS,     VIEW_SHIFT    ) |
        detail::bit_mask(KIND_BITS,     KIND_SHIFT    ) |
        detail::bit_mask(MATERIAL_BITS, MATERIAL_SHIFT);

    // Signed order range that fits the 24-bit field.
    static constexpr std::int32_t ORDER_MIN = -(1 << (ORDER_BITS - 1));
    static constexpr std::int32_t ORDER_MAX =  (1 << (ORDER_BITS - 1)) - 1;

    // -------------------------------------------------------------------------
    // Pack / Unpack
    // -------------------------------------------------------------------------

    static constexpr std::uint64_t make(
        const DrawParams& params,
        KindID            kind)
    {
        // Bias so the signed range maps onto 0..2^24-1 in the same order.
        const std::int32_t  clamped = std::clamp(params.order, ORDER_MIN, ORDER_MAX);
        const std::uint64_t order   = std::uint64_t(std::int64_t(clamped) - ORDER_MIN);

        return (std::uint64_t(params.view & 0xF) << VIEW_SHIFT    )
             | (std::uint64_t(params.layer)      << LAYER_SHIFT   )
             | (order                            << ORDER_SHIFT   )
             | (std::uint64_t(kind)              << KIND_SHIFT    )
             | (std::uint64_t(params.material)   << MATERIAL_SHIFT);
    }

    static constexpr std::uint8_t view(std::uint64_t key) {
        return std::uint8_t((key >> VIEW_SHIFT) & 0xF);
    }

    static constexpr KindID kind(std::uint64_t key) {
        return KindID((key >> KIND_SHIFT) & 0xFF);
    }

    static constexpr MaterialID material(std::uint64_t key) {
        return MaterialID((key >> MATERIAL_SHIFT) & 0xFFFF);
    }

    static constexpr bool same_state(std::uint64_t a, std::uint64_t b) {
        return ((a ^ b) & STATE_MASK) == 0;
    }
};

} // namespace Nezumi::Render
