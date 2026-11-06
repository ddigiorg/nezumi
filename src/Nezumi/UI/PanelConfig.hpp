// =============================================================================
// UI/PanelConfig.hpp
//
// How a panel (a window, menu or dialog inside the game window) looks and
// what the player can do with it. Passed to UI::Context::begin_panel() every
// frame; position and size only apply the first time a panel appears,
// unless PinPosition is set. After that the panel remembers where the player
// moved it and how big they made it.
//
//   // Debug menu: drag by the title, close with the X, sized to fit
//   const UI::PanelConfig debug = {
//       .position = { 16, 16 },
//       .flags    = UI::PanelFlags::Movable | UI::PanelFlags::AutoSize,
//   };
//
//   // Start screen: centered on the screen, no title bar, follows resizes
//   const UI::PanelConfig start = {
//       .position = ui.screen_size() * 0.5f,
//       .pivot    = { 0.5f, 0.5f },
//       .flags    = UI::PanelFlags::NoTitleBar | UI::PanelFlags::AutoSize |
//                   UI::PanelFlags::PinPosition,
//   };
//
//   // Tabs along the top, under the title (they don't scroll with the
//   // content); draw the one Context::panel_tab() says is picked
//   static constexpr std::string_view TABS[] = { "Inventory", "Crafting" };
//   if (ui.begin_panel("Settlement", &open, { .tabs = TABS })) {
//       if (ui.panel_tab() == 0) ...inventory...
//       else                     ...crafting...
//       ui.end_panel();
//   }
//
//   // Browser-style: an icon in place of the title and the tabs beside it,
//   // all in the title bar; drag the panel by the icon (or the bar's empty
//   // part), close it with the X
//   if (ui.begin_panel("Settlement", &open, { .tabs = TABS, .icon = banner })) ...
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include "Nezumi/UI/Image.hpp"
#include "Nezumi/UI/Rect.hpp"
#include <cstdint>     // uint8_t
#include <glm/glm.hpp> // vec2
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::UI {

enum class PanelFlags : std::uint8_t {
    None        = 0,
    Movable     = 1u << 0, // drag by the title bar
    Resizable   = 1u << 1, // drag any edge or corner
    AutoSize    = 1u << 2, // size to fit the content every frame (not resizable)
    NoTitleBar  = 1u << 3, // no title bar, so no close button and no dragging
    PinPosition = 1u << 4, // apply position and pivot every frame
    NoBackground = 1u << 5, // no background, border, shadow or padding: the game draws its own
};
ENUM_CLASS_FLAGS(PanelFlags);

struct PanelConfig {
    // Top-left corner in screen pixels (y down), adjusted by the pivot
    glm::vec2 position = { 32.0f, 32.0f };

    // Which point of the panel sits at `position`: { 0, 0 } top-left,
    // { 0.5, 0.5 } center, { 1, 1 } bottom-right
    glm::vec2 pivot    = { 0.0f, 0.0f };

    // Initial size, including the title bar (ignored with AutoSize)
    glm::vec2 size     = { 320.0f, 240.0f };

    // Smallest size the player can resize to
    glm::vec2 min_size = { 120.0f, 60.0f };

    PanelFlags flags   = PanelFlags::Movable;

    // Scrolling (panels that aren't AutoSize, once their content is taller
    // than they are): > 0 snaps the scroll to multiples of this, one per
    // mouse wheel notch, so a grid of slots scrolls by whole rows (pass its
    // row pitch: slot height plus spacing). 0: style.scroll_speed per notch,
    // anywhere. The scrollbar's thumb can be dragged either way, and a press
    // on its track jumps the thumb there.
    float scroll_step = 0.0f;

    // The part of the panel that takes the mouse, from its top-left corner;
    // empty (the default): all of it. Lets a panel with decorations that
    // stick out (an action bar's flanking statues) claim only its bar, so
    // clicks on the see-through parts reach the game.
    Rect hit_box = {};

    // Tabs in a strip under the title bar, left to right; empty: none.
    // Clicking one picks it (the content scrolls back to the top), and
    // Context::panel_tab() says which is picked. Must stay valid for the
    // begin_panel call.
    std::span<const std::string_view> tabs = {};

    // Shown in the title bar's left end in place of the title, which then
    // only names the panel (its id). The tabs move up into the title bar
    // beside it, like a browser's, and the panel is dragged by the icon or
    // the bar's empty part, not by the tabs. None: the title as text.
    Image icon = {};
};

} // namespace Nezumi::UI
