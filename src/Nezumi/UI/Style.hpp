// =============================================================================
// UI/Style.hpp
//
// Sizes and colors for every panel and widget. Plain data: change a field on
// UI::Context::style() and the next frame uses it. Sizes are in pixels.
//
// The defaults are Nezumi's own theme, after Tokyo Night and the tiling
// Linux desktops (Hyprland, Omarchy): flat square panels with one outline,
// a see-through background, magenta headings, cyan accents, and a cyan to
// magenta gradient outline on the panel in front. A game restyles the UI by
// changing these fields once at startup, e.g. from its own theme function:
//
//   UI::Style& style = engine.ui()->style();
//   style.accent         = { 0.35f, 0.65f, 1.0f, 1.0f };
//   style.panel_rounding = 4.0f; // softer corners
//   style.panel_bg.a     = 1.0f; // solid panels
//
// Colors with alpha 0 aren't drawn (title_bg, panel_shadow by default), so
// a theme that wants a filled title bar or a drop shadow just gives them one.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::UI {

struct Style {

    // -------------------------------------------------------------------------
    // Sizes
    // -------------------------------------------------------------------------

    float     font_size          = 16.0f;
    glm::vec2 panel_padding      = { 12.0f, 10.0f }; // inside the panel's edge
    glm::vec2 frame_padding      = {  8.0f,  4.0f }; // inside buttons, fields, ...
    glm::vec2 item_spacing       = {  8.0f,  6.0f }; // between widgets
    float     title_padding      = 6.0f;             // above and below the header's text
    float     tab_padding        = 10.0f;            // left and right of a tab's icon and label
    float     panel_rounding     = 0.0f;             // square, like Omarchy; try 4-8 for softer
    float     frame_rounding     = 0.0f;             // buttons, fields, slots, tooltips
    float     panel_border_width = 2.0f;             // the panel's outline
    float     border_width       = 1.0f;             // buttons, fields, slots, tooltips
    float     control_width      = 180.0f; // sliders, fields, dropdowns in AutoSize panels
    float     control_fraction   = 0.6f;   // ... and in sized panels, of the content width
    float     resize_margin      = 5.0f;   // grab zone on each side of a resizable edge
    float     scrollbar_width    = 4.0f;
    float     scroll_speed       = 40.0f;  // pixels per wheel notch
    float     tooltip_delay      = 0.4f;   // seconds of hover before a tooltip shows
    float     drag_threshold     = 4.0f;   // points the mouse moves before a press drags

    // Direction of the front panel's gradient outline, in degrees:
    // border_front at the start, border_front_end at the end. 45: from the
    // bottom-left corner to the top-right, like Hyprland's default.
    float     border_gradient_angle = 45.0f;

    // -------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------

    // Tokyo Night: blue-black panels, lavender text, magenta headings and
    // selections, cyan accents

    glm::vec4 text               = { 0.75f, 0.79f, 0.96f, 1.00f }; // #C0CAF5
    glm::vec4 text_dim           = { 0.45f, 0.48f, 0.64f, 1.00f }; // #737AA2
    glm::vec4 heading            = { 0.73f, 0.60f, 0.97f, 1.00f }; // #BB9AF7

    // Panels: see-through, so the game shows behind them
    glm::vec4 panel_bg           = { 0.10f, 0.11f, 0.15f, 0.86f }; // #1A1B26
    glm::vec4 panel_border       = { 0.23f, 0.26f, 0.38f, 1.00f }; // #3B4261: panels behind
    glm::vec4 border_front       = { 0.49f, 0.81f, 1.00f, 1.00f }; // #7DCFFF: the front panel's
    glm::vec4 border_front_end   = { 0.73f, 0.60f, 0.97f, 1.00f }; // #BB9AF7  outline, a gradient
    glm::vec4 panel_shadow       = { 0.00f, 0.00f, 0.00f, 0.00f }; // none
    glm::vec4 title_bg           = { 0.00f, 0.00f, 0.00f, 0.00f }; // header fill: none, a line under it
    glm::vec4 title_bg_front     = { 0.00f, 0.00f, 0.00f, 0.00f };
    glm::vec4 tab_hovered        = { 0.75f, 0.79f, 0.96f, 0.06f }; // a faint wash of text color
    glm::vec4 tab_picked         = { 0.49f, 0.81f, 1.00f, 0.08f }; // in a tab strip; underlined in accent

    // Fields, checkboxes, slider tracks, slots: sunken and tinted, so they
    // read on any background without stacking into a solid block
    glm::vec4 frame_bg           = { 0.09f, 0.09f, 0.12f, 0.55f }; // #16161E
    glm::vec4 frame_bg_hovered   = { 0.16f, 0.18f, 0.26f, 0.65f }; // #292E42
    glm::vec4 frame_bg_active    = { 0.23f, 0.26f, 0.38f, 0.70f };
    glm::vec4 frame_border       = { 0.23f, 0.26f, 0.38f, 1.00f }; // #3B4261
    glm::vec4 frame_border_hovered = { 0.34f, 0.37f, 0.54f, 1.00f }; // #565F89

    // Buttons: an outline over a faint fill that lights up in the accent
    glm::vec4 button             = { 0.09f, 0.09f, 0.12f, 0.35f };
    glm::vec4 button_hovered     = { 0.49f, 0.81f, 1.00f, 0.12f };
    glm::vec4 button_active      = { 0.49f, 0.81f, 1.00f, 0.25f };
    glm::vec4 button_border      = { 0.23f, 0.26f, 0.38f, 1.00f };
    glm::vec4 button_border_hovered = { 0.49f, 0.81f, 1.00f, 1.00f };
    glm::vec4 button_text_hovered   = { 0.49f, 0.81f, 1.00f, 1.00f };

    glm::vec4 accent             = { 0.49f, 0.81f, 1.00f, 1.00f }; // #7DCFFF: checks, grabs, focus
    glm::vec4 accent_dim         = { 0.25f, 0.42f, 0.55f, 1.00f }; // toggle track, idle grabs
    glm::vec4 close_hovered      = { 0.97f, 0.46f, 0.56f, 1.00f }; // #F7768E
    glm::vec4 close_text_hovered = { 0.10f, 0.11f, 0.15f, 1.00f };
    glm::vec4 separator          = { 0.23f, 0.26f, 0.38f, 1.00f }; // #3B4261
    glm::vec4 scrollbar          = { 0.34f, 0.37f, 0.54f, 0.90f }; // #565F89
    glm::vec4 selection          = { 0.73f, 0.60f, 0.97f, 0.30f }; // #BB9AF7, faint
    glm::vec4 tooltip_bg         = { 0.09f, 0.09f, 0.12f, 0.96f }; // #16161E
};

} // namespace Nezumi::UI
