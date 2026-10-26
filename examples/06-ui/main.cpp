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
// - "Bag" and "Hotbar" are slot grids (UI::Context::slot_grid) of icons cut
//   from the example atlas. Drag icons between slots and between the two
//   grids; drop one outside both to throw it away, right-click a bag slot
//   to empty it. Hovering a slot shows a tooltip.
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
#include <cstdint>     // uint32_t, uint64_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <span>        // span
#include <string>      // string
#include <string_view> // string_view
#include <utility>     // swap
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

// Slot grid icons: the arrow on each unit frame in assets/graphics/atlas.png
// (two rows of ten 64 px frames), 48x48 px squares
static constexpr std::size_t ICON_COUNT = 20;
static constexpr std::array<std::string_view, 4> ICON_COLORS = { "Red", "Purple", "Blue", "Teal" };
static constexpr std::array<std::string_view, 5> ICON_HEADINGS = {
    "down", "down-right", "right", "up-right", "up" };

// Drag type shared by both grids, so icons move between them
static constexpr std::uint32_t DRAG_ICON = 1;
static constexpr int EMPTY = -1;

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

        // Icons for the slot grids, and a full bag to start with
        const Render::Atlas& atlas = *renderer->create_atlas(
            Render::AtlasConfig{ .name = "example-atlas", .filepath = "assets/graphics/atlas.png" }).object;
        for (std::size_t i = 0; i < ICON_COUNT; i++) {
            const glm::vec2 frame = { 64.0f * float(i % 10), 128.0f * float(i / 10) };
            icons_[i] = UI::Image::from(atlas, { frame + glm::vec2(8.0f), frame + glm::vec2(56.0f) });
        }
        for (std::size_t i = 0; i < bag_.size(); i++) bag_[i] = static_cast<int>(i);
        hotbar_.fill(EMPTY);

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
            markers_.push_back(world_camera_->screen_to_world(engine.ui()->mouse() *
                                                                engine.ui()->scale()));

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
        draw_slot_panels_(ui);
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

    // Two slot grids sharing one drag type. Each grid reports drops onto its
    // slots; the example decides what a drop means (here: swap the two slots,
    // wherever the drag came from).
    void draw_slot_panels_(UI::Context& ui) {
        const auto slots_of = [&](std::span<const int> contents, std::span<UI::Slot> out) {
            for (std::size_t i = 0; i < contents.size(); i++) {
                out[i] = {};
                if (contents[i] == EMPTY) continue;
                out[i].icon  = icons_[static_cast<std::size_t>(contents[i])];
                out[i].value = static_cast<std::uint64_t>(contents[i]);
            }
        };
        const auto tooltip_for = [&](std::span<const int> contents, int hovered) {
            if (hovered < 0 || contents[static_cast<std::size_t>(hovered)] == EMPTY) return;
            const std::size_t icon = static_cast<std::size_t>(contents[static_cast<std::size_t>(hovered)]);
            char tip[64];
            std::snprintf(tip, sizeof tip, "%.*s %.*s arrow",
                int(ICON_COLORS[icon / 5].size()), ICON_COLORS[icon / 5].data(),
                int(ICON_HEADINGS[icon % 5].size()), ICON_HEADINGS[icon % 5].data());
            ui.tooltip(tip);
        };

        // Where a drop came from: a slot in one of the two grids
        const auto source_slot = [&](const UI::DragPayload& p) -> int* {
            if (p.index < 0) return nullptr;
            if (p.source == bag_id_    && p.index < int(bag_.size()))    return &bag_[std::size_t(p.index)];
            if (p.source == hotbar_id_ && p.index < int(hotbar_.size())) return &hotbar_[std::size_t(p.index)];
            return nullptr;
        };
        const auto drop_into = [&](int& target, const UI::DragPayload& p) {
            if (int* from = source_slot(p)) std::swap(target, *from);
        };

        // Bag: 4 x 5, movable
        const UI::PanelConfig bag_config = {
            .position = { ui.screen_size().x - 24.0f, ui.screen_size().y - 24.0f },
            .pivot    = { 1.0f, 1.0f },
            .flags    = UI::PanelFlags::Movable | UI::PanelFlags::AutoSize,
        };
        if (ui.begin_panel("Bag", bag_config)) {
            std::array<UI::Slot, 20> slots;
            slots_of(bag_, slots);
            bag_id_ = ui.get_id("bag");
            const UI::SlotGridResult r = ui.slot_grid("bag", slots, 5, {}, DRAG_ICON);
            if (r.dropped >= 0)       drop_into(bag_[std::size_t(r.dropped)], r.payload);
            if (r.right_clicked >= 0) bag_[std::size_t(r.right_clicked)] = EMPTY;
            tooltip_for(bag_, r.hovered);
            ui.end_panel();
        }

        // Hotbar: 1 x 5, pinned to the bottom center, keys 1-5 as labels
        static constexpr std::array<std::string_view, 5> KEYS = { "1", "2", "3", "4", "5" };
        const UI::PanelConfig bar_config = {
            .position = { ui.screen_size().x * 0.5f, ui.screen_size().y - 56.0f },
            .pivot    = { 0.5f, 1.0f },
            .flags    = UI::PanelFlags::NoTitleBar | UI::PanelFlags::AutoSize | UI::PanelFlags::PinPosition,
        };
        if (ui.begin_panel("Hotbar", bar_config)) {
            std::array<UI::Slot, 5> slots;
            slots_of(hotbar_, slots);
            for (std::size_t i = 0; i < slots.size(); i++) slots[i].label = KEYS[i];
            hotbar_id_ = ui.get_id("hotbar");
            const UI::SlotGridResult r = ui.slot_grid("hotbar", slots, 5, {}, DRAG_ICON);
            if (r.dropped >= 0) drop_into(hotbar_[std::size_t(r.dropped)], r.payload);
            tooltip_for(hotbar_, r.hovered);
            ui.end_panel();
        }

        // Dropped where neither grid took it: thrown away
        if (const auto drop = ui.take_unclaimed_drop(DRAG_ICON))
            if (int* from = source_slot(drop->payload)) *from = EMPTY;
    }

    void draw_status_(Engine& engine, UI::Context& ui) {
        // Stats are for the last completed frame. The UI is one draw call per
        // texture run (see UIQuad.hpp): text, then the slot icons, then text
        // again, so the count barely moves as panels open.
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

    // Slot grids: an icon index per slot, or EMPTY
    std::array<UI::Image, ICON_COUNT> icons_     = {};
    std::array<int, 20>               bag_       = {};
    std::array<int, 5>                hotbar_    = {};
    UI::Context::ID                   bag_id_    = 0;
    UI::Context::ID                   hotbar_id_ = 0;

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
