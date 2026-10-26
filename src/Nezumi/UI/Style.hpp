// =============================================================================
// UI/Style.hpp
//
// Sizes and colors for every panel and widget. Plain data: change a field on
// UI::Context::style() and the next frame uses it. Sizes are in pixels.
//
// The defaults are Nezumi's warm dark theme. A game restyles the UI by
// changing these fields once at startup, e.g. from its own theme function:
//
//   UI::Style& style = engine.ui()->style();
//   style.accent = { 0.35f, 0.65f, 1.0f, 1.0f };
//
// Making the UI pretty later (textures, nine-slice frames, per-panel styles)
// builds on this rather than replacing it.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::UI {

struct Style {

    // -------------------------------------------------------------------------
    // Sizes
    // -------------------------------------------------------------------------

    float     font_size        = 16.0f;
    glm::vec2 panel_padding    = { 10.0f, 8.0f }; // inside the panel's edge
    glm::vec2 frame_padding    = {  8.0f, 4.0f }; // inside buttons, fields, ...
    glm::vec2 item_spacing     = {  8.0f, 6.0f }; // between widgets
    float     title_padding    = 6.0f;            // above and below the title
    float     panel_rounding   = 4.0f;
    float     frame_rounding   = 3.0f;
    float     border_width     = 1.0f;
    float     control_width    = 180.0f; // sliders, fields, dropdowns in AutoSize panels
    float     control_fraction = 0.6f;   // ... and in sized panels, of the content width
    float     resize_margin    = 5.0f;   // grab zone on each side of a resizable edge
    float     scrollbar_width  = 6.0f;
    float     scroll_speed     = 40.0f;  // pixels per wheel notch
    float     tooltip_delay    = 0.4f;   // seconds of hover before a tooltip shows
    float     drag_threshold   = 4.0f;   // points the mouse moves before a press drags

    // -------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------

    // Warm charcoal with cyan accents, and purple (GitHub's "merged"
    // #8250df family) on headings and selections

    glm::vec4 text               = { 0.93f, 0.92f, 0.89f, 1.00f }; // #ECEAE3
    glm::vec4 text_dim           = { 0.64f, 0.63f, 0.58f, 1.00f }; // #A3A094
    glm::vec4 heading            = { 0.64f, 0.44f, 0.97f, 1.00f }; // #A371F7

    glm::vec4 panel_bg           = { 0.15f, 0.15f, 0.14f, 0.96f }; // #262624
    glm::vec4 panel_border       = { 0.08f, 0.08f, 0.07f, 1.00f }; // #141413
    glm::vec4 title_bg           = { 0.19f, 0.19f, 0.18f, 1.00f }; // #30302E
    glm::vec4 title_bg_front     = { 0.24f, 0.24f, 0.23f, 1.00f }; // #3D3D3A

    glm::vec4 frame_bg           = { 0.19f, 0.19f, 0.18f, 1.00f };
    glm::vec4 frame_bg_hovered   = { 0.23f, 0.23f, 0.22f, 1.00f };
    glm::vec4 frame_bg_active    = { 0.27f, 0.27f, 0.26f, 1.00f };
    glm::vec4 frame_border       = { 0.08f, 0.08f, 0.07f, 1.00f };

    glm::vec4 button             = { 0.24f, 0.24f, 0.23f, 1.00f };
    glm::vec4 button_hovered     = { 0.29f, 0.29f, 0.28f, 1.00f };
    glm::vec4 button_active      = { 0.17f, 0.17f, 0.16f, 1.00f };

    glm::vec4 accent             = { 0.22f, 0.77f, 0.81f, 1.00f }; // #39C5CF: checks, grabs, focus
    glm::vec4 accent_dim         = { 0.14f, 0.50f, 0.54f, 1.00f }; // toggle track, idle grabs
    glm::vec4 close_hovered      = { 0.75f, 0.24f, 0.22f, 1.00f };
    glm::vec4 separator          = { 0.24f, 0.24f, 0.23f, 1.00f };
    glm::vec4 scrollbar          = { 0.42f, 0.42f, 0.40f, 0.80f };
    glm::vec4 selection          = { 0.35f, 0.23f, 0.60f, 1.00f }; // #8250df, dimmed
    glm::vec4 tooltip_bg         = { 0.10f, 0.10f, 0.09f, 0.96f };
};

} // namespace Nezumi::UI
