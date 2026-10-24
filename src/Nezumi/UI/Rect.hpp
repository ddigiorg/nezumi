// =============================================================================
// UI/Rect.hpp
//
// Axis-aligned box in UI coordinates: pixels, origin at the top-left of the
// screen, y down (the usual convention for UI layout; the UI flips to the
// renderer's y-up screen view only when it submits draws).
// =============================================================================

#pragma once
#include <algorithm>   // max, min
#include <glm/glm.hpp> // vec2

namespace Nezumi::UI {

struct Rect {
    glm::vec2 min = { 0.0f, 0.0f }; // top-left
    glm::vec2 max = { 0.0f, 0.0f }; // bottom-right

    static constexpr Rect from_size(glm::vec2 pos, glm::vec2 size) {
        return { pos, pos + size };
    }

    constexpr float     width () const { return max.x - min.x; }
    constexpr float     height() const { return max.y - min.y; }
    constexpr glm::vec2 size  () const { return max - min;     }
    constexpr glm::vec2 center() const { return (min + max) * 0.5f; }

    // Half-open, so two touching rects never both contain a point
    constexpr bool contains(glm::vec2 p) const {
        return p.x >= min.x && p.y >= min.y && p.x < max.x && p.y < max.y;
    }

    constexpr Rect expand(float by) const {
        return { min - glm::vec2(by), max + glm::vec2(by) };
    }

    constexpr Rect shrink(glm::vec2 by) const {
        return { min + by, max - by };
    }

    // Overlap of two rects (empty, with max <= min, if they don't overlap)
    constexpr Rect intersect(const Rect& o) const {
        return { { std::max(min.x, o.min.x), std::max(min.y, o.min.y) },
                 { std::min(max.x, o.max.x), std::min(max.y, o.max.y) } };
    }
};

} // namespace Nezumi::UI
