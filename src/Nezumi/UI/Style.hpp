// =============================================================================
// UI/Style.hpp
//
// Sizes and colors for every panel and widget. Plain data: change a field on
// UI::Context::style() and the next frame uses it. Sizes are in pixels.
//
// The defaults are a plain dark theme. Making the UI pretty later (textures,
// nine-slice frames, per-panel styles) builds on this rather than replacing
// it.
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

    // -------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------

    glm::vec4 text               = { 0.92f, 0.92f, 0.92f, 1.00f };
    glm::vec4 text_dim           = { 0.60f, 0.60f, 0.60f, 1.00f };
    glm::vec4 heading            = { 1.00f, 0.86f, 0.62f, 1.00f };

    glm::vec4 panel_bg           = { 0.14f, 0.14f, 0.15f, 0.96f };
    glm::vec4 panel_border       = { 0.05f, 0.05f, 0.05f, 1.00f };
    glm::vec4 title_bg           = { 0.20f, 0.20f, 0.22f, 1.00f };
    glm::vec4 title_bg_front     = { 0.26f, 0.26f, 0.29f, 1.00f };

    glm::vec4 frame_bg           = { 0.24f, 0.24f, 0.26f, 1.00f };
    glm::vec4 frame_bg_hovered   = { 0.30f, 0.30f, 0.33f, 1.00f };
    glm::vec4 frame_bg_active    = { 0.36f, 0.36f, 0.40f, 1.00f };
    glm::vec4 frame_border       = { 0.08f, 0.08f, 0.08f, 1.00f };

    glm::vec4 button             = { 0.32f, 0.32f, 0.36f, 1.00f };
    glm::vec4 button_hovered     = { 0.40f, 0.40f, 0.45f, 1.00f };
    glm::vec4 button_active      = { 0.24f, 0.24f, 0.27f, 1.00f };

    glm::vec4 accent             = { 1.00f, 0.66f, 0.20f, 1.00f }; // checks, grabs, focus
    glm::vec4 accent_dim         = { 0.55f, 0.38f, 0.14f, 1.00f }; // toggle track when on
    glm::vec4 close_hovered      = { 0.80f, 0.22f, 0.20f, 1.00f };
    glm::vec4 separator          = { 0.30f, 0.30f, 0.32f, 1.00f };
    glm::vec4 scrollbar          = { 0.45f, 0.45f, 0.48f, 0.80f };
    glm::vec4 selection          = { 0.35f, 0.30f, 0.20f, 1.00f };
};

} // namespace Nezumi::UI
