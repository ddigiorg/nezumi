// =============================================================================
// BatchBuilder.hpp
//
// Turns the frame's submitted commands into draw runs:
//
//   1. Sort the commands by key (stable).
//   2. Walk them, cutting a new run wherever the state part of the key
//      (view / kind / material) changes.
//   3. Gather each run's instance bytes from the CommandQueue arena into the
//      StreamBuffer, contiguously and in sorted order.
//
// Step 3 is why submission goes through an arena: after sorting, a run's
// items are scattered through the arena, and a draw needs them packed.
//
// Owns no GPU state; the Renderer records the runs.
// =============================================================================

#pragma once
#include "Nezumi/Render/Frame/CommandQueue.hpp"
#include "Nezumi/Render/Frame/DrawBatch.hpp"
#include "Nezumi/Render/Frame/DrawCommand.hpp"
#include "Nezumi/Render/Frame/StreamBuffer.hpp"
#include <cstdint> // uint32_t
#include <span>    // span
#include <vector>  // vector

namespace Nezumi::Render {

class BatchBuilder {
public:
    struct Stats {
        std::uint32_t objects = 0; // commands submitted
        std::uint32_t batches = 0; // draw calls produced
        std::uint32_t dropped = 0; // items that didn't fit in GPU memory
    };

    // Sorts `queue` in place, fills `stream`, and returns the runs (valid
    // until the next build).
    std::span<const DrawBatch> build(CommandQueue& queue, StreamBuffer& stream);

    const Stats& stats() const { return stats_; }

private:
    std::vector<DrawCommand> scratch_; // radix sort ping-pong buffer
    std::vector<DrawBatch>   batches_;
    Stats                    stats_;
};

} // namespace Nezumi::Render
