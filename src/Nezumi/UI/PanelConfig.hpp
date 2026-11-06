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
//   // Tabs: every panel has one header row, left to right: its icon and
//   // title (both optional), its tabs, empty space to drag it by, and the
//   // close button. Tabs that don't fit there move to a strip of their own
//   // under the header (TabPlacement::Auto). Draw the one panel_tab() says
//   // is picked; the content scrolls back to the top when it changes.
//   static constexpr std::string_view TABS[] = { "Inventory", "Crafting" };
//   if (ui.begin_panel("Settlement", &open, { .tabs = TABS, .icon = banner })) {
//       if (ui.panel_tab() == 0) ...inventory...
//       else                     ...crafting...
//       ui.end_panel();
//   }
//
//   // Browser-style: the icon alone names the panel, the tabs beside it
//   ui.begin_panel("Settlement", &open, { .flags = UI::PanelFlags::Movable |
//       UI::PanelFlags::HideTitle, .tabs = TABS, .icon = banner });
//
//   // Tabs with icons (from a sprite sheet), or icons only (empty labels)
//   static constexpr std::string_view LABELS[] = { "", "", "" };
//   const UI::Image icons[] = { bag, hammer, scroll };
//   ui.begin_panel("Pack", &open, { .tabs = LABELS, .tab_icons = icons });
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include "Nezumi/UI/Image.hpp"
#include "Nezumi/UI/Rect.hpp"
#include <cstdint>     // uint8_t, uint16_t
#include <glm/glm.hpp> // vec2
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::UI {

enum class PanelFlags : std::uint16_t {
    None        = 0,
    Movable     = 1u << 0, // drag by the title bar
    Resizable   = 1u << 1, // drag any edge or corner
    AutoSize    = 1u << 2, // size to fit the content every frame (not resizable)
    NoTitleBar  = 1u << 3, // no title bar, so no close button and no dragging
    PinPosition = 1u << 4, // apply position and pivot every frame
    NoBackground = 1u << 5, // no background, border, shadow or padding: the game draws its own
    HideTitle   = 1u << 6, // the header shows the icon but not the title text
};
ENUM_CLASS_FLAGS(PanelFlags);

// Where a panel's tabs go
enum class TabPlacement : std::uint8_t {
    Auto   = 0, // in the header when they fit beside the icon, title and
                // close button, else in a strip under it
    Header = 1, // always in the header (the panel widens to fit them)
    Strip  = 2, // always in a strip under the header
};

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

    // Tabs, left to right; empty: none. Clicking one picks it (the content
    // scrolls back to the top), and Context::panel_tab() says which is
    // picked. Must stay valid for the begin_panel call. An empty label
    // with an icon makes an icon-only tab.
    std::span<const std::string_view> tabs = {};

    // An icon in front of each tab's label, in the same order as tabs. May
    // be shorter than tabs, and an invalid Image means no icon.
    std::span<const Image> tab_icons = {};

    TabPlacement tab_placement = TabPlacement::Auto;

    // At the header's left end, before the title. None: no icon.
    Image icon = {};
};

} // namespace Nezumi::UI
