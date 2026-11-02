// =============================================================================
// examples/08-sprite-sheet/main.cpp
//
// A SpriteSheet loaded from a manifest (assets/graphics/example-sheet.json):
// two pages in one Texture2DArray, named sprites with pivots, and animations.
//
// - Top: the logo, a sprite on page 1.
// - Middle: four figures from page 0 turning through 8 headings. Only 5
//   directions are stored; the 3 facing left are the right-facing ones
//   mirrored ("headings": 8 in the manifest).
// - Bottom: the same figures as a looping 5-frame animation at 2 fps.
//
// Everything is one material, so the whole scene is one sprite draw call even
// though it spans two pages. Mouse wheel zooms (the sheet has 4 mip levels).
// Escape quits.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Engine.hpp"
#include <array>       // array
#include <cmath>       // floor
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique

using namespace Nezumi;

class SpriteSheetExample final : public IApplication {
public:

    void on_init(Engine& engine) override {
        Render::Renderer2D& r = *engine.renderer2d();

        sheet_ = r.create_sprite_sheet({
            .name       = "example-sheet",
            .manifest   = "assets/graphics/example-sheet.json",
            .mip_levels = 4, // sprites sit on a 64 px grid with 8 px margins
        }).object;

        // Names -> ids once, at load
        logo_ = sheet_->sprite("logo");
        for (std::size_t i = 0; i < COLORS.size(); ++i) {
            const std::string base = std::string("figure/") + COLORS[i];
            turn_[i]  = sheet_->animation(base + "/turn");
            cycle_[i] = sheet_->animation(base + "/cycle");
        }

        font_ = r.create_font({ .name = "example-font",
                                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf" }).object;

        screen_camera_     = r.get_view_camera(Render::View::Screen);
        camera_controller_ = std::make_unique<CameraController>(
            engine.event_bus(), engine.input(), r, r.get_view(Render::View::World),
            CameraControllerConfig{ .enable_key_pan = false, .zoom_min = 0.25f, .zoom_max = 4.0f });

        engine.logger().info("SpriteSheet example: " + std::to_string(sheet_->sprite_count()) + " sprites, " +
                             std::to_string(sheet_->animation_count()) + " animations on " +
                             std::to_string(sheet_->pages()) + " pages");
    }

    void on_tick(Engine&) override {}

    void on_draw(Engine& engine) override {
        Render::Renderer2D& r = *engine.renderer2d();
        camera_controller_->update(engine.clock().get_real_dt());
        seconds_ += engine.clock().get_real_dt();

        const Render::DrawParams params = { .material = sheet_->material() };

        // Logo (page 1), pivot at its bottom center
        r.draw_sprite(sheet_->sprite_at(logo_, { 0.0f, 40.0f }, 0.5f), params);

        // Turning figures: one heading step every 0.75 s
        const auto heading = static_cast<std::uint32_t>(std::floor(seconds_ / 0.75f));
        for (std::size_t i = 0; i < COLORS.size(); ++i) {
            const glm::vec2 feet = { -240.0f + 160.0f * float(i), -100.0f };
            r.draw_sprite(sheet_->sprite_at(sheet_->frame(turn_[i], heading + std::uint32_t(i), 0.0f),
                                            feet, 1.0f), params);
        }

        // Cycling figures, tinted by their row
        for (std::size_t i = 0; i < COLORS.size(); ++i) {
            const glm::vec2 feet = { -240.0f + 160.0f * float(i), -230.0f };
            Render::Sprite s = sheet_->sprite_at(sheet_->frame(cycle_[i], 0, seconds_ + 0.25f * float(i)),
                                                 feet, 0.75f);
            s.tint = Render::rgba8(1.0f, 1.0f, 1.0f, 0.5f + 0.5f * float(i) / 3.0f);
            r.draw_sprite(s, params);
        }

        // HUD
        char text[128];
        std::snprintf(text, sizeof text, "heading %u of 8 (5 stored, 3 mirrored)", (heading % 8u));
        const float top = screen_camera_->get_size().y / screen_camera_->get_scale() - 8.0f;
        r.draw_text(text, { 8.0f, top }, *font_,
                    Render::TextStyle{ .size = 16.0f, .valign = Render::TextVAlign::Top },
                    { .view = Render::View::Screen });
    }

    void on_exit(Engine&) override {
        camera_controller_.reset();
    }

private:

    static constexpr std::array<const char*, 4> COLORS = { "red", "purple", "blue", "cyan" };

    Render::SpriteSheet* sheet_ = nullptr;
    Render::Font*        font_  = nullptr;
    Render::Camera2D*    screen_camera_ = nullptr;
    std::unique_ptr<CameraController> camera_controller_;

    Render::SpriteID                      logo_  = 0;
    std::array<Render::AnimationID, 4>    turn_  = {};
    std::array<Render::AnimationID, 4>    cycle_ = {};
    float                                 seconds_ = 0.0f;
};

int main() {
    try {
        EngineConfig config = {};
        config.window.title = "Nezumi: SpriteSheet";
        config.escape_exits = true;

        Engine engine(config);
        SpriteSheetExample app;
        engine.run(app);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
