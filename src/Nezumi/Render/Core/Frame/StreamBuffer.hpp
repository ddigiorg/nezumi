// =============================================================================
// StreamBuffer.hpp
//
// Chunked GPU memory buffer allocator.
//
// A pool of fixed-size chunks (default 1 MB) to submit vertex data into.
// Nothing is ever resized or copied. When a chunk is full, allocation moves to
// the next one, creating it only the first time a frame needs that much.
//
// Each chunk has a CPU-side mirror. The BatchBuilder writes instances into
// the mirror, and flush() uploads each chunk's used range with ONE
// write_buffer call. Every byte is written at most once per frame, which is
// exactly what Device::write_buffer's upload rule asks for.
//
// Cross-frame hazards (the GPU still reading last frame's chunk) are handled
// by the device, per the write_buffer contract. If profiling ever shows
// stalls there, give each frame in flight its own chunk set; that's an
// internal change to this class only.
//
//   Frame N
//
//   CPU Chunk 0                            CPU Chunk 1
//   +------------+----------+--------+     +--------+
//   | 50 sprites | 20 rects | unused |  +  | unused |  +  etc.
//   +------------+----------+--------+     +--------+
//   Batch 0      Batch 1
//
//   flush() uploads data from CPU to GPU
//
//   GPU Chunk 0                            GPU Chunk 1
//   +------------+----------+--------+     +--------+
//   | 50 sprites | 20 rects | unused |  +  | unused |  +  etc.
//   +------------+----------+--------+     +--------+
//
// TODO: A persistently mapped stream buffer. The builder would write straight
// into GPU-visible memory, removing two of the three data copies (mirror,
// GL staging, and glBufferSubData).
// =============================================================================

#pragma once
#include "Nezumi/RHI/RHI.hpp"
#include <cstddef> // byte, size_t
#include <cstdint> // uint32_t, uint64_t
#include <vector>  // vector

namespace Nezumi::Render {

class StreamBuffer {
public:

    // -------------------------------------------------------------------------
    // Allocation
    //
    // Where a group of instances landed. count may be less than requested if
    // the current chunk had room for only some of them; ask again for the rest.
    // -------------------------------------------------------------------------

    struct Allocation {
        RHI::BufferHandle buffer = {};
        std::uint64_t     offset = 0;       // byte offset in buffer
        std::byte*        data   = nullptr; // CPU mirror to write into
        std::uint32_t     count  = 0;       // instances that fit; 0 = failure
    };

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    StreamBuffer(RHI::Device& device, std::size_t chunk_size);
    ~StreamBuffer();

    // Non-copyable and non-movable
    StreamBuffer(const StreamBuffer&)            = delete;
    StreamBuffer& operator=(const StreamBuffer&) = delete;
    StreamBuffer(StreamBuffer&&)                 = delete;
    StreamBuffer& operator=(StreamBuffer&&)      = delete;

    // -------------------------------------------------------------------------
    // Per-frame Usage
    // -------------------------------------------------------------------------

    // Forget last frame's allocations. Chunks are kept.
    void reset();

    // Room for up to `count` instances of `stride` bytes, contiguous, starting
    // at a multiple of ALIGN. Returns count == 0 only if a single instance
    // can't fit in an empty chunk, or a new chunk couldn't be created.
    Allocation allocate(std::uint32_t stride, std::uint32_t count);

    // Upload every used range. Call once, after all allocations.
    void flush();

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    static constexpr std::size_t ALIGN = 16;

    std::size_t chunk_size()  const { return chunk_size_;    }
    std::size_t chunk_count() const { return chunks_.size(); }
    std::size_t bytes_used()  const;

private:

    // -------------------------------------------------------------------------
    // Chunk
    // -------------------------------------------------------------------------

    struct Chunk {
        RHI::BufferHandle      buffer;
        std::vector<std::byte> mirror;
        std::size_t            used = 0;
    };

    bool add_chunk_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device&       device_;
    std::size_t        chunk_size_;
    std::vector<Chunk> chunks_;
    std::size_t        current_ = 0; // chunk being allocated from
};

} // namespace Nezumi::Render
