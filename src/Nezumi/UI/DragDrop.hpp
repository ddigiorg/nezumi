// =============================================================================
// UI/DragDrop.hpp
//
// What is being dragged, and where it landed. Drag and drop is immediate
// mode like the rest of the UI: a source starts a drag when the player holds
// the mouse on it and moves, a target asks each frame whether a drag is over
// it, and the frame the button is released, the target under the cursor
// receives the payload. See Context::begin_drag() and Context::drop_target().
//
// The payload is plain data. `type` says what kind of thing it is, so a
// target only lights up for drags it can take (an action bar takes items and
// skills, a trash can takes items). `value` is the game's own: an item id,
// a skill id, an index. `source` and `index` say where the drag came from,
// filled in by slot_grid() for its slots, so the target can swap.
//
//   enum : std::uint32_t { DragItem = 1 };
//
//   // Source: the payload follows the cursor, drawn as `image`
//   if (ui.begin_drag(id, rect, { .type = DragItem, .value = item, .image = icon })) ...
//
//   // Target: highlight while hovered, take it on release
//   const UI::DropResult drop = ui.drop_target(rect, DragItem);
//   if (drop.hovered) ...highlight...
//   if (drop.payload) put(*drop.payload);
//
// A drag released anywhere no target took it (over the world, or a panel
// with no target there) isn't lost: the next frame,
// take_unclaimed_drop(type) hands it to whoever wants it. An action bar
// clears the slot it came from; an inventory drops the item on the ground.
// =============================================================================

#pragma once
#include "Nezumi/UI/Image.hpp"
#include <cstdint>     // uint32_t, uint64_t
#include <glm/glm.hpp> // vec2
#include <optional>    // optional

namespace Nezumi::UI {

struct DragPayload {
    std::uint32_t type   = 0;  // the game's kind of thing; 0 is never dropped
    std::uint64_t value  = 0;  // the game's data (an item id, ...)
    std::uint32_t source = 0;  // id of the widget it came from (Context::get_id)
    int           index  = -1; // slot it came from, for slot_grid; -1 otherwise
    Image         image  = {}; // drawn under the cursor while dragging
};

struct DropResult {
    bool                       hovered = false; // a drag of this type is over the target
    std::optional<DragPayload> payload = {};    // released on the target this frame
};

// A drag no target took, the frame after its release
struct UnclaimedDrop {
    DragPayload payload  = {};
    glm::vec2   position = {};    // cursor at the release, UI units (y down)
    bool        on_panel = false; // released over a panel, not the game world
};

} // namespace Nezumi::UI
