// =============================================================================
// Rgba8.hpp
//
// A color packed into 4 bytes (r in the lowest byte), as instance data reads
// it with VertexFormat::Unorm8x4. A quarter of a glm::vec4, for per-instance
// tints that stay within 0..1.
// =============================================================================

#pragma once
#include <algorithm>   // clamp
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec4

namespace Nezumi::Render {

// Each channel clamped to 0..1 and rounded to the nearest of 256 steps
constexpr std::uint32_t rgba8(float r, float g, float b, float a = 1.0f) {
    const auto byte = [](float c) {
        const float x = std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f;
        return static_cast<std::uint32_t>(x);
    };
    return byte(r) | (byte(g) << 8) | (byte(b) << 16) | (byte(a) << 24);
}

constexpr std::uint32_t rgba8(const glm::vec4& c) {
    return rgba8(c.r, c.g, c.b, c.a);
}

inline constexpr std::uint32_t RGBA8_WHITE = 0xFFFFFFFFu;

} // namespace Nezumi::Render
