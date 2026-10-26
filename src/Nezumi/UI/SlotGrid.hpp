// =============================================================================
// UI/SlotGrid.hpp
//
// A grid of item slots: action bars, inventories, equipment, chests, shops.
// Each slot is a frame with an optional icon, a hotkey label in the top-right
// corner, a count in the bottom-right, and a cooldown shade. The player can
// click a slot, right-click it, and drag icons between slots (and between
// grids that share a drag type).
//
//   std::array<UI::Slot, 10> slots;
//   for (int i = 0; i < 10; i++) {
//       slots[i].icon  = icon_of(bar[i]);
//       slots[i].label = key_names[i];
//       slots[i].value = bar[i];
//   }
//   const UI::SlotGridResult r = ui.slot_grid("bar", slots, 10, style, DragItem);
//   if (r.clicked >= 0) use(bar[r.clicked]);
//   if (r.dropped >= 0) move(r.payload, r.dropped);
//
// The grid knows nothing about items: the game owns what is in each slot,
// shows it through Slot, and decides what a click or a drop does. A drop
// onto a slot is reported, not applied, so the game decides whether it is a
// swap, a merge or not allowed.
// =============================================================================

#pragma once
#include "Nezumi/UI/DragDrop.hpp"
#include "Nezumi/UI/Image.hpp"
#include <cstdint>     // uint32_t, uint64_t
#include <glm/glm.hpp> // vec2, vec4
#include <string_view> // string_view

namespace Nezumi::UI {

// What one slot shows. Default: an empty slot.
struct Slot {
    Image            icon     = {};    // none: empty slot (nothing to drag)
    std::string_view label    = {};    // top-right, e.g. the hotkey "1"
    std::string_view count    = {};    // bottom-right, e.g. a stack size "20"
    float            cooldown = 0.0f;  // 0..1 of the slot shaded, from the top
    bool             flash    = false; // drawn pressed, e.g. while its hotkey is held
    bool             disabled = false; // icon dimmed; still draggable
    std::uint64_t    value    = 0;     // DragPayload::value when dragged from here
};

// How the slots look. Defaults draw plain boxes in the UI's colors, so a grid
// works before the game has any art.
struct SlotStyle {
    glm::vec2 slot_size   = { 40.0f, 40.0f };
    glm::vec2 spacing     = {  2.0f,  2.0f };

    // Under the icon, and over it (e.g. a bevelled rim that overlaps the
    // icon's edge). Either can be left empty.
    NineSlice frame       = {};
    NineSlice frame_over  = {};

    // The icon sits inside the slot by `icon_inset` points on each side,
    // its corners rounded by `icon_radius` (the slot's mask)
    float     icon_inset  = 3.0f;
    float     icon_radius = 3.0f;

    // Text: the label in the top-right corner, the count in the bottom-right
    float     label_size  = 11.0f;
    glm::vec4 label_color = { 0.85f, 0.88f, 0.92f, 1.0f };
    glm::vec4 count_color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Overlays, over the icon
    glm::vec4 hover       = { 1.0f, 1.0f, 1.0f, 0.12f }; // cursor over the slot
    glm::vec4 pressed     = { 0.0f, 0.0f, 0.0f, 0.30f }; // held, or Slot::flash
    glm::vec4 drop_hover  = { 1.0f, 1.0f, 1.0f, 0.22f }; // a drag it takes is over it
    glm::vec4 highlight   = { 0.0f, 0.0f, 0.0f, 0.00f }; // border color while hovered (a = 0: none)
    glm::vec4 cooldown    = { 0.0f, 0.0f, 0.0f, 0.55f };
    float     disabled_tint = 0.4f; // icon brightness when disabled
};

// What the player did this frame; each slot index is -1 when nothing happened
struct SlotGridResult {
    int         hovered       = -1; // under the cursor
    int         clicked       = -1; // left click (press and release on it, no drag)
    int         right_clicked = -1;
    int         drag_started  = -1; // a drag left this slot this frame
    int         dragging      = -1; // a drag from this grid is in flight, from this slot
    int         dropped       = -1; // a drag was released on this slot ...
    DragPayload payload       = {}; // ... carrying this (check payload.source / index)
};

} // namespace Nezumi::UI
