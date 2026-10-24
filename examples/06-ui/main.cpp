// =============================================================================
// examples/06-ui/main.cpp
//
// This example shows the UI: panels and every widget, declared each frame
// in on_draw with the game's own variables as their state.
//
// - "Widgets" is movable and resizable, and scrolls when its content doesn't
//   fit (mouse wheel over it).
// - "Settings" sizes itself to its content (AutoSize) and can only be moved.
// - "Main Menu" has no title bar and stays centered on the screen, even when
//   the window is resized (PinPosition with a pivot).
// - Behind the panels is a world view: left click drops a marker, the wheel
//   zooms and middle drag pans. While the cursor is over a panel the UI
//   claims the mouse, so clicks, wheel and drags go to the panel instead.
//
// Controls: left click to use widgets or drop markers, wheel to scroll a
// panel or zoom, middle drag to pan, F1 to show all panels again. Press
// Escape (or close the window) to quit.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Core/Events/OnGameExited.hpp"
#include "Nezumi/Engine.hpp"
#include <array>       // array
#include <cmath>       // sin
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <string>      // string
#include <string_view> // string_view
#include <vector>      // vector

using namespace Nezumi;

// -----------------------------------------------------------------------------
// Input Actions
// -----------------------------------------------------------------------------

namespace Action {
    inline constexpr ActionID PlaceMarker = 0;
    inline constexpr ActionID ShowPanels  = 1;
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

static constexpr float TILE_PIXELS = 32.0f;

static constexpr std::array<std::string_view, 4> COLOR_NAMES = { "Red", "Green", "Blue", "Gold" };
static constexpr std::array<glm::vec4, 4> COLORS = {{
    { 0.85f, 0.25f, 0.22f, 1.0f },
    { 0.30f, 0.75f, 0.35f, 1.0f },
    { 0.30f, 0.50f, 0.95f, 1.0f },
    { 0.95f, 0.75f, 0.25f, 1.0f },
}};

static constexpr std::array<std::string_view, 3> DIFFICULTIES = { "Easy", "Normal", "Nightmare" };

// -----------------------------------------------------------------------------
// UIExample
// -----------------------------------------------------------------------------

class UIExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Engine& engine) override {
        Render::Renderer* renderer = engine.renderer();

        engine.input().bind(Action::PlaceMarker, MouseButton::Left);
        engine.input().bind(Action::ShowPanels,  ScanCode::F1);

        world_camera_ = renderer->get_view_camera(Render::View::World);
        world_camera_->set_pixels_per_unit(TILE_PIXELS);

        camera_controller_ = std::make_unique<CameraController>(
            /*.events   = */engine.event_bus(),
            /*.input    = */engine.input(),
            /*.renderer = */*renderer,
            /*.camera   = */renderer->get_view(Render::View::World),
            CameraControllerConfig{
                .enable_key_pan = false,
            }
        );

        engine.logger().info("UI Example");
        engine.logger().info("Left click: use widgets / drop markers. Wheel: scroll or zoom. F1: show panels.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // The InputMap never sees a press the UI claimed, so a click on a button
    // doesn't also drop a marker behind it.
    // -------------------------------------------------------------------------

    void on_tick(Engine& engine) override {
        if (engine.input().was_pressed(Action::PlaceMarker))
            markers_.push_back(world_camera_->screen_to_world(engine.ui()->mouse()));

        if (engine.input().was_pressed(Action::ShowPanels)) {
            show_widgets_  = true;
            show_settings_ = true;
            show_menu_     = true;
        }

        if (quit_requested_) engine.event_bus().dispatch(OnGameExited{});
        time_ += 1.0f / 60.0f;
    }

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Engine& engine) override {
        camera_controller_->update(engine.clock().get_real_dt());

        draw_world_(*engine.renderer());

        UI::Context& ui = *engine.ui();
        draw_widgets_panel_(ui);
        draw_settings_panel_(ui);
        draw_main_menu_(ui);
        draw_status_(engine, ui);
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Engine& engine) override {
        camera_controller_.reset();
        engine.logger().info("Goodbye!");
    }

private:

    // -------------------------------------------------------------------------
    // Panels
    // -------------------------------------------------------------------------

    void draw_widgets_panel_(UI::Context& ui) {
        const UI::PanelConfig config = {
            .position = { 24.0f, 24.0f },
            .size     = { 380.0f, 470.0f },
            .min_size = { 220.0f, 120.0f },
            .flags    = UI::PanelFlags::Movable | UI::PanelFlags::Resizable,
        };
        if (!ui.begin_panel("Widgets", &show_widgets_, config)) return;

        ui.heading("Buttons");
        if (ui.button("Click me")) clicks_++;
        ui.same_line();
        char line[64];
        std::snprintf(line, sizeof line, "clicked %d times", clicks_);
        ui.text(line);
        if (ui.button("Reset##clicks")) clicks_ = 0;

        ui.separator();
        ui.heading("Choices");
        ui.checkbox("Show squares", show_squares_);
        ui.checkbox("Show markers", show_markers_);
        ui.toggle("Animate", animate_);
        ui.dropdown("Color", color_, COLOR_NAMES);

        ui.separator();
        ui.heading("Values");
        ui.slider("Square size", square_size_, 0.2f, 1.0f);
        ui.slider("Grid radius", grid_radius_, 1, 12);

        ui.separator();
        ui.heading("Text");
        ui.text_field("Name", name_, 24);
        std::snprintf(line, sizeof line, "Hello, %s!", name_.empty() ? "stranger" : name_.c_str());
        ui.text(line, ui.style().text_dim);

        ui.separator();
        ui.heading("Scrolling");
        ui.text("Resize this panel or scroll it");
        ui.text("with the mouse wheel.");
        for (int i = 1; i <= 6; i++) {
            std::snprintf(line, sizeof line, "Line %d", i);
            ui.text(line);
        }
        if (ui.button("Clear markers", -1.0f)) markers_.clear();

        ui.end_panel();
    }

    void draw_settings_panel_(UI::Context& ui) {
        const UI::PanelConfig config = {
            .position = { ui.screen_size().x - 24.0f, 24.0f },
            .pivot    = { 1.0f, 0.0f }, // top-right corner at position
            .flags    = UI::PanelFlags::Movable | UI::PanelFlags::AutoSize,
        };
        if (!ui.begin_panel("Settings", &show_settings_, config)) return;

        ui.heading("Video");
        ui.toggle("Fullscreen", fullscreen_);
        ui.toggle("VSync", vsync_);

        ui.spacing();
        ui.heading("Audio");
        ui.slider("Master", volume_, 0, 100);
        ui.slider("Music", music_, 0, 100);

        ui.spacing();
        ui.heading("Game");
        ui.dropdown("Difficulty", difficulty_, DIFFICULTIES);

        ui.spacing();
        if (ui.button("Apply"))  show_settings_ = false;
        ui.same_line();
        if (ui.button("Cancel")) show_settings_ = false;

        ui.end_panel();
    }

    void draw_main_menu_(UI::Context& ui) {
        if (!show_menu_) return;

        const UI::PanelConfig config = {
            .position = ui.screen_size() * glm::vec2(0.5f, 0.62f),
            .pivot    = { 0.5f, 0.5f },
            .min_size = { 200.0f, 0.0f },
            .flags    = UI::PanelFlags::NoTitleBar | UI::PanelFlags::AutoSize |
                        UI::PanelFlags::PinPosition,
        };
        if (!ui.begin_panel("Main Menu", config)) return;

        ui.heading("NEZUMI");
        ui.spacing();
        if (ui.button("Continue", -1.0f)) show_menu_ = false;
        if (ui.button("Settings", -1.0f)) show_settings_ = true;
        if (ui.button("Quit",     -1.0f)) quit_requested_ = true;

        ui.end_panel();
    }

    void draw_status_(Engine& engine, UI::Context& ui) {
        // Stats are for the last completed frame. The whole UI is one draw
        // call (see UIQuad.hpp), so the count barely moves as panels open.
        const Render::Renderer::FrameStats& stats = engine.renderer()->stats();

        char text[200];
        std::snprintf(text, sizeof text,
            "ui wants mouse: %-3s  keyboard: %-3s  markers: %zu  draw calls: %u  objects: %u\n"
            "left click: markers  wheel: zoom  middle drag: pan  F1: show panels",
            ui.wants_mouse() ? "yes" : "no", ui.wants_keyboard() ? "yes" : "no",
            markers_.size(), stats.draw_calls, stats.objects);

        engine.renderer()->draw_text(
            text, { 12.0f, 12.0f }, *ui.font(),
            Render::TextStyle{
                .size          = 15.0f,
                .outline_color = { 0.0f, 0.0f, 0.0f, 1.0f },
                .outline_width = 3.0f,
                .valign        = Render::TextVAlign::Bottom,
            },
            { .view = Render::View::Screen }
        );
    }

    // -------------------------------------------------------------------------
    // World
    // -------------------------------------------------------------------------

    void draw_world_(Render::Renderer& r) {
        const glm::vec4 color = COLORS[static_cast<std::size_t>(color_)];

        if (show_squares_) {
            for (int y = -grid_radius_; y <= grid_radius_; y++) {
                for (int x = -grid_radius_; x <= grid_radius_; x++) {
                    float s = square_size_;
                    if (animate_) s *= 0.75f + 0.25f * std::sin(time_ * 3.0f + float(x + y) * 0.5f);
                    const glm::vec2 c = { float(x) * 1.25f, float(y) * 1.25f };
                    r.draw_rectangle({ .pos_box = { c - 0.5f * s, c + 0.5f * s }, .color = color });
                }
            }
        }

        if (show_markers_) {
            for (const glm::vec2& m : markers_) {
                r.draw_circle({ .position = m, .color = { 1.0f, 1.0f, 1.0f, 1.0f }, .radius = 0.3f },
                              { .layer = 1 });
            }
        }
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Render::Camera*                   world_camera_      = nullptr;
    std::unique_ptr<CameraController> camera_controller_ = nullptr;

    // Panel visibility
    bool show_widgets_  = true;
    bool show_settings_ = true;
    bool show_menu_     = true;

    // Widget state: plain members, the UI keeps no copy
    int         clicks_       = 0;
    bool        show_squares_ = true;
    bool        show_markers_ = true;
    bool        animate_      = false;
    int         color_        = 3;
    float       square_size_  = 0.8f;
    int         grid_radius_  = 6;
    std::string name_         = "Nezumi";

    bool fullscreen_ = false;
    bool vsync_      = true;
    int  volume_     = 80;
    int  music_      = 60;
    int  difficulty_ = 1;

    std::vector<glm::vec2> markers_ = {};
    bool  quit_requested_ = false;
    float time_           = 0.0f;
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title  = "UI Example";
        config.window.width  = 1280;
        config.window.height = 720;
        config.escape_exits  = true;

        Nezumi::Engine engine(config);

        UIExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
