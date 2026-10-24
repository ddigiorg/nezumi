// =============================================================================
// UI/UIConfig.hpp
//
// Config of the UI context the Engine creates (EngineConfig::ui).
// =============================================================================

#pragma once
#include "Nezumi/Render/View/ViewID.hpp"
#include "Nezumi/UI/Style.hpp"
#include <cstdint>     // uint8_t, uint32_t
#include <string_view> // string_view

namespace Nezumi::UI {

struct UIConfig {
    // Font for all UI text, loaded once when the engine starts (about a
    // second; games can reuse it through UI::Context::font() instead of
    // loading their own). Empty disables the UI: begin_panel() returns false.
    std::string_view font_path       = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf";
    std::uint32_t    font_pixel_size = 48;

    // Where the UI draws: a BottomLeft, 1 unit = 1 pixel view, on the top
    // layer so it covers the game's own HUD
    Render::ViewID view  = Render::View::Screen;
    std::uint8_t   layer = 255;

    Style style = {};
};

} // namespace Nezumi::UI
