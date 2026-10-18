// =============================================================================
// CommandQueue.hpp
//
// Per-frame list of submitted draws. Submitting copies the item's GPU instance
// bytes into a linear arena and records a DrawCommand pointing at them; no GPU
// work happens until the BatchBuilder runs at the end of the frame.
//
// Both the arena and the command list keep their capacity across frames, so
// after the first few frames submission does no allocation at all.
// =============================================================================

#pragma once
#include "Nezumi/Render/Frame/DrawCommand.hpp"
#include <cstddef>     // byte, size_t
#include <cstdint>     // uint64_t
#include <cstring>     // memcpy
#include <span>        // span
#include <type_traits> // is_trivially_copyable_v
#include <vector>      // vector

namespace Nezumi::Render {

class CommandQueue {
public:
    // Every instance starts on this boundary in the arena. Covers vec4 / mat4
    // members; static_asserted against each kind's Instance in Renderer.
    static constexpr std::size_t ARENA_ALIGN = 16;

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit CommandQueue(
        std::size_t reserve_bytes    = 1 << 20, //  1 MB
        std::size_t reserve_commands = 1 << 14) // 16 KB
    {
        arena_.reserve(reserve_bytes);
        commands_.reserve(reserve_commands);
    }

    // -------------------------------------------------------------------------
    // Push
    // -------------------------------------------------------------------------

    template <typename T>
    void push(std::uint64_t key, const T& instance) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Instance data must be trivially copyable");
        push_bytes(key, std::as_bytes(std::span(&instance, 1)));
    }

    void push_bytes(std::uint64_t key, std::span<const std::byte> bytes) {
        const std::size_t offset = align_up_(arena_.size());
        arena_.resize(offset + bytes.size());
        std::memcpy(arena_.data() + offset, bytes.data(), bytes.size());

        commands_.push_back(DrawCommand{
            .key    = key,
            .offset = static_cast<std::uint32_t>(offset),
            .size   = static_cast<std::uint32_t>(bytes.size()),
        });
    }

    // -------------------------------------------------------------------------
    // Clear
    // -------------------------------------------------------------------------

    void clear() {
        arena_.clear();    // keeps capacity
        commands_.clear(); // keeps capacity
    }

    // -------------------------------------------------------------------------
    // Info
    // -------------------------------------------------------------------------

    std::span<DrawCommand>       commands()       { return commands_; }
    std::span<const DrawCommand> commands() const { return commands_; }

    const std::byte* data_at(std::uint32_t offset) const {
        return arena_.data() + offset;
    }

    bool        empty()      const { return commands_.empty(); }
    std::size_t size()       const { return commands_.size();  }
    std::size_t arena_size() const { return arena_.size();     }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static std::size_t align_up_(std::size_t n) {
        return (n + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::vector<std::byte>   arena_;
    std::vector<DrawCommand> commands_;
};

} // namespace Nezumi::Render
