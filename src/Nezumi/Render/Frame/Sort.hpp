// =============================================================================
// Sort.hpp
//
// Stable LSD radix sort of DrawCommands by their 64-bit key, one byte per pass.
// Items with identical keys keep submission order, so overlapping items with
// the same layer/order never flicker between frames.
//
// Passes where every key has the same byte are skipped. That's common here:
// the view and spare bits are constant, and often so are kind/material, so a
// typical frame runs 3-5 passes instead of 8.
// =============================================================================

#pragma once
#include "Nezumi/Render/Frame/DrawCommand.hpp"
#include <algorithm> // copy
#include <array>     // array
#include <cstddef>   // size_t
#include <cstdint>   // uint32_t, uint64_t
#include <span>      // span
#include <utility>   // swap
#include <vector>    // vector

namespace Nezumi::Render {

// Note: `scratch` is resized to match and reused across frames by the caller.
inline void radix_sort(
    std::span<DrawCommand>    commands,
    std::vector<DrawCommand>& scratch)
{
    const std::size_t n = commands.size();
    if (n < 2) return;

    scratch.resize(n);

    // One histogram per byte, filled in a single read of the data.
    std::array<std::array<std::uint32_t, 256>, 8> counts{};
    for (const DrawCommand& c : commands) {
        for (uint32_t b = 0; b < 8; ++b) {
            ++counts[b][(c.key >> (b * 8)) & 0xFF];
        }
    }

    DrawCommand* src = commands.data();
    DrawCommand* dst = scratch.data();

    for (uint32_t b = 0; b < 8; ++b) {
        std::array<std::uint32_t, 256>& count = counts[b];

        // Every key has the same value in this byte: nothing to reorder.
        const std::uint32_t first_byte = std::uint32_t((src[0].key >> (b * 8)) & 0xFF);
        if (count[first_byte] == n) continue;

        // Counts -> starting offsets.
        std::uint32_t sum = 0;
        for (std::uint32_t& c : count) {
            const std::uint32_t here = c;
            c = sum;
            sum += here;
        }

        for (std::size_t i = 0; i < n; ++i) {
            const std::uint32_t byte = std::uint32_t((src[i].key >> (b * 8)) & 0xFF);
            dst[count[byte]++] = src[i];
        }

        std::swap(src, dst);
    }

    // An odd number of passes left the result in scratch.
    if (src != commands.data()) {
        std::copy(src, src + n, commands.data());
    }
}

} // namespace Nezumi::Render
