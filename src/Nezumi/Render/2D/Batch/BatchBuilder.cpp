// =============================================================================
// BatchBuilder.cpp
// =============================================================================

#include "Nezumi/Render/2D/Batch/BatchBuilder.hpp"
#include "Nezumi/Render/Core/Frame/Sort.hpp"
#include "Nezumi/Render/2D/Batch/SortKey.hpp"
#include <cassert> // assert
#include <cstring> // memcpy

namespace Nezumi::Render {

std::span<const DrawBatch> BatchBuilder::build(
    CommandQueue& queue,
    StreamBuffer& stream)
{
    batches_.clear();

    stats_ = Stats{
        .objects = static_cast<std::uint32_t>(queue.size())
    };

    std::span<DrawCommand> commands = queue.commands();

    radix_sort(commands, scratch_);

    std::size_t i = 0;
    while (i < commands.size()) {

        // Find the end of the state group starting at i.
        const std::uint64_t key = commands[i].key;
        std::size_t end = i + 1;
        while (end < commands.size() && SortKey::same_state(commands[end].key, key)) {
            ++end;
        }

        const std::uint32_t stride = commands[i].size;

        // Copy the group into GPU memory. Usually one allocation; more if the
        // group crosses a chunk boundary, each becoming its own run.
        while (i < end) {
            const StreamBuffer::Allocation alloc = stream.allocate(
                /*.stride = */stride,
                /*.count  = */static_cast<std::uint32_t>(end - i)
            );

            if (alloc.count == 0) {
                // Out of GPU memory (logged by the device), or an instance
                // larger than a chunk. Drop the rest of this group.
                stats_.dropped += static_cast<std::uint32_t>(end - i);
                i = end;
                break;
            }

            std::byte* out = alloc.data;
            for (std::uint32_t n = 0; n < alloc.count; ++n, ++i) {
                assert(commands[i].size == stride && "one kind, one instance size");
                std::memcpy(out, queue.data_at(commands[i].offset), stride);
                out += stride;
            }

            batches_.push_back(DrawBatch{
                .view           = SortKey::view(key),
                .kind           = SortKey::kind(key),
                .material       = SortKey::material(key),
                .buffer         = alloc.buffer,
                .byte_offset    = alloc.offset,
                .instance_count = alloc.count,
            });
        }
    }

    stats_.batches = static_cast<std::uint32_t>(batches_.size());

    return batches_;
}

} // namespace Nezumi::Render
