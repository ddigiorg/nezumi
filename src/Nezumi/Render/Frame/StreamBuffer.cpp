// =============================================================================
// StreamBuffer.cpp
// =============================================================================

#include "Nezumi/Render/Frame/StreamBuffer.hpp"
#include "Nezumi/RHI/Buffer/BufferConfig.hpp"
#include <algorithm> // min
#include <span>      // span

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Align Up
// -----------------------------------------------------------------------------

namespace {

std::size_t align_up(std::size_t n, std::size_t a) {
    return (n + a - 1) & ~(a - 1);
}

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

StreamBuffer::StreamBuffer(RHI::Device& device, std::size_t chunk_size)
    : device_    (device)
    , chunk_size_(align_up(chunk_size, ALIGN))
{}

StreamBuffer::~StreamBuffer() {
    for (const Chunk& chunk : chunks_) {
        device_.remove(chunk.buffer);
    }
}

// -----------------------------------------------------------------------------
// Per-frame Usage
// -----------------------------------------------------------------------------

void StreamBuffer::reset() {
    for (Chunk& chunk : chunks_) {
        chunk.used = 0;
    }
    current_ = 0;
}

StreamBuffer::Allocation StreamBuffer::allocate(
    std::uint32_t stride,
    std::uint32_t count)
{
    if (stride == 0 || count == 0 || stride > chunk_size_) return {};

    for (;;) {
        if (current_ == chunks_.size() && !add_chunk_()) return {};

        Chunk&            chunk = chunks_[current_];
        const std::size_t start = align_up(chunk.used, ALIGN);
        const std::size_t room  = start < chunk_size_ ? chunk_size_ - start : 0;
        const std::size_t fits  = std::min<std::size_t>(room / stride, count);

        if (fits == 0) {
            ++current_; // this chunk is full for this stride; try the next
            continue;
        }

        chunk.used = start + fits * stride;

        return Allocation{
            .buffer = chunk.buffer,
            .offset = start,
            .data   = chunk.mirror.data() + start,
            .count  = static_cast<std::uint32_t>(fits),
        };
    }
}

void StreamBuffer::flush() {
    for (const Chunk& chunk : chunks_) {
        if (chunk.used == 0) continue;
        device_.write_buffer(
            /*.buffer = */chunk.buffer,
            /*.offset = */0,
            /*.data   = */std::span<const std::byte>(chunk.mirror.data(), chunk.used)
        );
    }
}

// -------------------------------------------------------------------------
// Info
// -------------------------------------------------------------------------

std::size_t StreamBuffer::bytes_used() const {
    std::size_t total = 0;
    for (const Chunk& chunk : chunks_) total += chunk.used;
    return total;
}

// -----------------------------------------------------------------------------
// Add Chunk
// -----------------------------------------------------------------------------

bool StreamBuffer::add_chunk_() {
    const std::string name = "stream chunk " + std::to_string(chunks_.size());

    const RHI::BufferHandle buffer = device_.create(
        RHI::BufferConfig{
            .name  = name,
            .size  = chunk_size_,
            .usage = RHI::BufferUsageFlags::Vertex |
                     RHI::BufferUsageFlags::CopyDst,
        }
    );

    if (!buffer) return false; // logged by the device

    chunks_.push_back(
        Chunk{
            .buffer = buffer,
            .mirror = std::vector<std::byte>(chunk_size_),
            .used   = 0,
        }
    );

    return true;
}

} // namespace Nezumi::Render
