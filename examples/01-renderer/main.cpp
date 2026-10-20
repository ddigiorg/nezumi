// =============================================================================
// examples/01-renderer/main.cpp
//
// This example shows how to use the Nezumi engine renderer.
//
// Press Escape (or close the window) to quit.
// =============================================================================

#include "Nezumi/Engine.hpp"
#include "Nezumi/Render/View/ViewID.hpp"
#include <array>       // array
#include <cmath>       // cos, sin
#include <cstdint>     // int32_t, uint32_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <glm/glm.hpp> // vec2, vec4, mix
#include <iostream>    // cerr
#include <numbers>     // numbers::pi_v
#include <string_view> // string_view

using namespace Nezumi; // TODO: should examples be more explicit?

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

// Convert degrees to radians
constexpr float radians(float degrees) {
    return degrees * (std::numbers::pi_v<float> / 180.0f);
}

// Point on a circle of `radius` around `center` at `angle` (radians).
glm::vec2 on_circle_(glm::vec2 center, float radius, float angle) {
    return center + radius * glm::vec2{ std::cos(angle), std::sin(angle) };
}

// -----------------------------------------------------------------------------
// Palette
// -----------------------------------------------------------------------------

namespace Palette {
    constexpr glm::vec4 White       = { 1.00f, 1.00f, 1.00f, 1.00f };
    constexpr glm::vec4 Black       = { 0.00f, 0.00f, 0.00f, 1.00f };
    constexpr glm::vec4 Clear       = { 0.00f, 0.00f, 0.00f, 0.00f };
    constexpr glm::vec4 Red         = { 0.90f, 0.20f, 0.20f, 1.00f };
    constexpr glm::vec4 Green       = { 0.20f, 0.80f, 0.30f, 1.00f };
    constexpr glm::vec4 Blue        = { 0.20f, 0.40f, 0.90f, 1.00f };
    constexpr glm::vec4 Yellow      = { 0.95f, 0.85f, 0.20f, 1.00f };
    constexpr glm::vec4 Cyan        = { 0.00f, 1.00f, 1.00f, 1.00f };
    constexpr glm::vec4 Magenta     = { 1.00f, 0.00f, 1.00f, 1.00f };
    constexpr glm::vec4 Lavender    = { 0.90f, 0.80f, 1.00f, 1.00f };
    constexpr glm::vec4 Purple      = { 0.30f, 0.20f, 0.50f, 1.00f };
    constexpr glm::vec4 GlassWhite  = { 1.00f, 1.00f, 1.00f, 0.35f };
}

// -----------------------------------------------------------------------------
// Layer
// -----------------------------------------------------------------------------

using Layer = decltype(Render::DrawParams::layer);

// Layers used by this example (higher draws on top)
static constexpr Layer LAYER_BACKGROUND = 0;
static constexpr Layer LAYER_GRID       = 1;
static constexpr Layer LAYER_CONTENT    = 2;
static constexpr Layer LAYER_OVERLAY    = 3;

// -----------------------------------------------------------------------------
// Draw Params
//
// One per layer this example draws on. Sprites add their atlas material.
// -----------------------------------------------------------------------------

constexpr Render::DrawParams DRAW_PARAMS_BACKGROUND = {
    .view  = Render::View::World,
    .layer = LAYER_BACKGROUND
};

constexpr Render::DrawParams DRAW_PARAMS_GRID = {
    .view  = Render::View::World,
    .layer = LAYER_GRID
};

constexpr Render::DrawParams DRAW_PARAMS_CONTENT = {
    .view  = Render::View::World,
    .layer = LAYER_CONTENT
};

constexpr Render::DrawParams DRAW_PARAMS_OVERLAY = {
    .view  = Render::View::World,
    .layer = LAYER_OVERLAY
};

constexpr Render::DrawParams DRAW_PARAMS_UI = {
    .view  = Render::View::Screen,
    .layer = 0
};

// -----------------------------------------------------------------------------
// RendererExample
// -----------------------------------------------------------------------------

class RendererExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Engine& engine) override {
        Render::Renderer* r = engine.renderer();

        // The renderer makes a camera for each default view: View::World
        // (centered on the origin) and View::Screen (pixels, origin
        // bottom-left). Both are sized to the window every frame.
        world_camera_  = r->get_view_camera(Render::View::World);
        screen_camera_ = r->get_view_camera(Render::View::Screen);

        // Setup sprite atlas
        atlas_ = r->create_atlas(
            Render::AtlasConfig{
                .name     = "example-atlas",
                .filepath = "assets/graphics/atlas.png",
            }
        ).object;

        // Setup font
        font_ = r->create_font(
            Render::FontConfig{
                .name     = "example-font",
                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf"
            }
        ).object;

        engine.logger().info("RendererExample initialized");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // No work required.
    // -------------------------------------------------------------------------

    void on_tick(Engine& engine) override {}

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Engine& engine) override {
        Render::Renderer* r = engine.renderer();

        draw_menu_(r);
        draw_arrows_(r);
        draw_circles_(r);
        draw_lines_(r);
        draw_rectangles_(r);
        draw_sdf_rectangles_(r);
        draw_sprites_(r);
        draw_sdf_texts_(r);
        draw_stats_(r, engine.get_stats(), r->stats());
    }

    // -------------------------------------------------------------------------
    // On Exit
    // -------------------------------------------------------------------------

    void on_exit(Engine& engine) override {
        engine.logger().info("RendererExample exited");
    }

private:

    // -------------------------------------------------------------------------
    // Draw Subtitle
    // -------------------------------------------------------------------------

    void draw_subtitle_(
        Render::Renderer* r,
        std::string_view  text,
        glm::vec2         position
    ) {
        // Section subtitle (white, black outline, centered).
        r->draw_text(
            /*.text     = */text,
            /*.position = */position,
            /*.font     = */*font_,
            Render::TextStyle {
                .size          = 24.0f,
                .color         = Palette::White,
                .outline_color = Palette::Black,
                .outline_width = 4.0f,
                .align         = Render::TextAlign::Center,
                .valign        = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );
    }

    // -------------------------------------------------------------------------
    // Draw Menu
    // -------------------------------------------------------------------------

    void draw_menu_(Render::Renderer* r) {
        static constexpr float TILE_SIZE = 32.0f;
        static constexpr float TILE_HALF = TILE_SIZE * 0.5f;

        static constexpr std::array<glm::vec4, 2> GRID_COLORS = {
            glm::vec4{ 0.10f, 0.10f, 0.10f, 1.0f }, // dark grey
            glm::vec4{ 0.18f, 0.18f, 0.18f, 1.0f }, // light grey
        };

        // Menu Background
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = { -376.0f, -280.0f, 376.0f, 280.0f },
                .color         = Palette::Purple,
                .border_color  = Palette::Clear,
                .corner_radius = 0.0f,
                .border_width  = 4.0f,
            },
            DRAW_PARAMS_BACKGROUND
        );

        // Menu Inline
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = { -368.0f, -272.0f, 368.0f, 272.0f },
                .color         = Palette::Clear,
                .border_color  = Palette::Black,
                .corner_radius = 0.0f,
                .border_width  = 2.0f,
            },
            DRAW_PARAMS_GRID
        );

        // Grid
        for (int32_t y = -8; y <= 8; y++) {
            for (int32_t x = -11; x <= 11; x++) {
                uint32_t i = static_cast<uint32_t>(x + 11 + y + 8) % 2;
                float cx = static_cast<float>(x) * TILE_SIZE;
                float cy = static_cast<float>(y) * TILE_SIZE;

                r->draw_rectangle(
                    Render::Rectangle{
                        .pos_box = {
                            cx - TILE_HALF, cy - TILE_HALF,
                            cx + TILE_HALF, cy + TILE_HALF,
                        },
                        .color   = GRID_COLORS[i],
                    },
                    DRAW_PARAMS_GRID,
                    /*.filled = */true
                );
            }
        }

        // Title
        r->draw_text(
            /*.text     = */"Nezumi Engine",
            /*.position = */{ 0.0f, 240.0f },
            /*.font     = */*font_,
            Render::TextStyle {
                .size           = 48.0f,
                .color          = Palette::Lavender,
                .weight         = 1.0f,
                .outline_color  = Palette::Black,
                .outline_width  = 4.0f,
                .outline_offset = { 0.0f, 0.0f },
                .line_spacing   = 1.0f,
                .align          = Render::TextAlign::Center,
                .valign         = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );
    }

    // -------------------------------------------------------------------------
    // Draw Arrows
    //
    // Eight arrows evenly spaced on a ring, each pointing outward, blending
    // color from cyan to magenta.
    // -------------------------------------------------------------------------

    void draw_arrows_(Render::Renderer* r) {
        static constexpr glm::vec2 CENTER = { -280.0f, 100.0f };
        static constexpr float     RADIUS = 40.0f;
        static constexpr int32_t   COUNT  = 8;

        draw_subtitle_(r, "Arrows", { CENTER.x, 0.0f });

        for (int32_t i = 0; i < COUNT; i++) {
            float t     = static_cast<float>(i) / static_cast<float>(COUNT - 1);
            float angle = radians(360.0f / COUNT * static_cast<float>(i));

            r->draw_arrow(
                Render::Arrow{
                    .position = on_circle_(CENTER, RADIUS, angle), // arrow center
                    .size     = 32.0f,
                    .angle    = angle,
                    .color    = glm::mix(Palette::Cyan, Palette::Magenta, t),
                },
                DRAW_PARAMS_CONTENT
            );
        }
    }

    // -------------------------------------------------------------------------
    // Draw Circles
    // -------------------------------------------------------------------------

    void draw_circles_(Render::Renderer* r) {
        static constexpr glm::vec2 CENTER = { -90.0f, 100.0f };

        draw_subtitle_(r, "Circles", { CENTER.x, 0.0f });

        // Circle: outline-only
        r->draw_circle(
            Render::Circle{
                .position = CENTER,
                .color    = Palette::Lavender,
                .radius   = 48.0f,
                .width    = 4.0f,
            },
            DRAW_PARAMS_CONTENT,
            /*.filled = */false
        );

        // Circle: filled
        r->draw_circle(
            Render::Circle{
                .position = CENTER,
                .color    = Palette::Purple,
                .radius   = 32.0f,
            },
            DRAW_PARAMS_CONTENT,
            /*.filled = */true
        );

        // Circle: filled transparent
        r->draw_circle(
            Render::Circle{
                .position = CENTER + glm::vec2{ 30.0f, 30.0f },
                .color    = { 0.0f, 1.0f, 1.0f, 0.25f },
                .radius   = 20.0f,
            },
            DRAW_PARAMS_OVERLAY,
            /*.filled = */true
        );
    }

    // -------------------------------------------------------------------------
    // Draw Lines
    //
    // Eight spokes on a ring that grow from 1px to 8px wide while blending
    // color from cyan to magenta.
    // -------------------------------------------------------------------------

    void draw_lines_(Render::Renderer* r) {
        static constexpr glm::vec2 CENTER       = { 90.0f, 100.0f };
        static constexpr float     INNER_RADIUS = 16.0f;
        static constexpr float     OUTER_RADIUS = 52.0f;
        static constexpr int32_t   COUNT        = 8;

        draw_subtitle_(r, "Lines", { CENTER.x, 0.0f });

        for (int32_t i = 0; i < COUNT; i++) {
            float t     = static_cast<float>(i) / static_cast<float>(COUNT - 1);
            float angle = radians(360.0f / COUNT * static_cast<float>(i));

            r->draw_line(
                Render::Line{
                    .position_a = on_circle_(CENTER, INNER_RADIUS, angle),
                    .position_b = on_circle_(CENTER, OUTER_RADIUS, angle),
                    .color      = glm::mix(Palette::Cyan, Palette::Magenta, t),
                    .width      = 1.0f + static_cast<float>(i),
                },
                DRAW_PARAMS_CONTENT
            );
        }
    }

    // -------------------------------------------------------------------------
    // Draw Rectangles
    //
    // A 2x2 grid of filled rectangles (red, green, blue, yellow), a translucent
    // white rectangle overlapping all four, and an outline-only frame.
    // -------------------------------------------------------------------------

    void draw_rectangles_(Render::Renderer* r) {
        static constexpr glm::vec2 CENTER = { 280.0f, 100.0f };
        static constexpr float     SIZE   = 36.0f; // each quadrant
        static constexpr float     GAP    = 4.0f;  // half gap between quadrants

        draw_subtitle_(r, "Rectangles", { CENTER.x, 0.0f });

        struct Quad { float sx, sy; glm::vec4 color; };
        static constexpr std::array<Quad, 4> QUADS = {{
            { -1.0f,  1.0f, Palette::Red    }, // top-left
            {  1.0f,  1.0f, Palette::Green  }, // top-right
            { -1.0f, -1.0f, Palette::Blue   }, // bottom-left
            {  1.0f, -1.0f, Palette::Yellow }, // bottom-right
        }};

        // Filled quadrants
        for (const Quad& q : QUADS) {
            float x0 = CENTER.x + q.sx * GAP;
            float y0 = CENTER.y + q.sy * GAP;
            float x1 = CENTER.x + q.sx * (GAP + SIZE);
            float y1 = CENTER.y + q.sy * (GAP + SIZE);

            r->draw_rectangle(
                Render::Rectangle{
                    .pos_box = { std::min(x0, x1), std::min(y0, y1),
                                 std::max(x0, x1), std::max(y0, y1) },
                    .color   = q.color,
                },
                DRAW_PARAMS_CONTENT,
                /*.filled = */true
            );
        }

        // Translucent overlay straddling all four quadrants
        r->draw_rectangle(
            Render::Rectangle{
                .pos_box = { CENTER.x - 20.0f, CENTER.y - 20.0f,
                             CENTER.x + 20.0f, CENTER.y + 20.0f },
                .color   = Palette::GlassWhite,
            },
            DRAW_PARAMS_OVERLAY,
            /*.filled = */true
        );

        // Outline-only frame around the group
        static constexpr float FRAME = GAP + SIZE + 6.0f;
        r->draw_rectangle(
            Render::Rectangle{
                .pos_box = { CENTER.x - FRAME, CENTER.y - FRAME,
                             CENTER.x + FRAME, CENTER.y + FRAME },
                .color   = Palette::White,
            },
            DRAW_PARAMS_CONTENT,
            /*.filled = */false
        );
    }

    // -------------------------------------------------------------------------
    // Draw SDF Rectangles
    //
    // Same color scheme as the plain rectangles, but each one shows off a
    // different SDF feature: sharp corners, rounded corners, a pill shape and
    // border-only. A translucent rounded overlay sits in the middle.
    // -------------------------------------------------------------------------

    void draw_sdf_rectangles_(Render::Renderer* r) {
        static constexpr glm::vec2 CENTER = { -250.0f, -160.0f };
        static constexpr float     HALF_W = 36.0f; // half width of each box
        static constexpr float     HALF_H = 20.0f; // half height of each box
        static constexpr float     STEP_X = 44.0f; // offset from CENTER
        static constexpr float     STEP_Y = 26.0f;

        draw_subtitle_(r, "SDF Rectangles", { CENTER.x, -250.0f });

        auto box = [](float cx, float cy, float hw, float hh) {
            return glm::vec4{ cx - hw, cy - hh, cx + hw, cy + hh };
        };

        // Sharp corners, thin white border
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = box(CENTER.x - STEP_X, CENTER.y + STEP_Y, HALF_W, HALF_H),
                .color         = Palette::Red,
                .border_color  = Palette::White,
                .corner_radius = 0.0f,
                .border_width  = 2.0f,
            },
            DRAW_PARAMS_CONTENT
        );

        // Rounded corners, thick black border
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = box(CENTER.x + STEP_X, CENTER.y + STEP_Y, HALF_W, HALF_H),
                .color         = Palette::Green,
                .border_color  = Palette::Black,
                .corner_radius = 8.0f,
                .border_width  = 4.0f,
            },
            DRAW_PARAMS_CONTENT
        );

        // Pill (corner radius = half height), no border
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = box(CENTER.x - STEP_X, CENTER.y - STEP_Y, HALF_W, HALF_H),
                .color         = Palette::Blue,
                .border_color  = Palette::Clear,
                .corner_radius = HALF_H,
                .border_width  = 0.0f,
            },
            DRAW_PARAMS_CONTENT
        );

        // Border only (transparent fill)
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = box(CENTER.x + STEP_X, CENTER.y - STEP_Y, HALF_W, HALF_H),
                .color         = Palette::Clear,
                .border_color  = Palette::Yellow,
                .corner_radius = 12.0f,
                .border_width  = 3.0f,
            },
            DRAW_PARAMS_CONTENT
        );

        // Translucent rounded overlay with a soft lavender border
        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box       = box(CENTER.x, CENTER.y, 24.0f, 24.0f),
                .color         = Palette::GlassWhite,
                .border_color  = Palette::Lavender,
                .corner_radius = 10.0f,
                .border_width  = 2.0f,
            },
            DRAW_PARAMS_OVERLAY
        );
    }

    // -------------------------------------------------------------------------
    // Draw Sprites
    //
    // The same atlas region (the red down arrow in the top-left corner of
    // atlas.png) drawn three times: untinted, tinted lavender, and translucent.
    // -------------------------------------------------------------------------

    void draw_sprites_(Render::Renderer* r) {
        static constexpr glm::vec4 PIXEL_BOX = { 0.0f, 0.0f, 64.0f, 128.0f };

        draw_subtitle_(r, "Sprites", { 0.0f, -250.0f });

        const glm::vec4 tex_box = atlas_->tex_box(PIXEL_BOX);

        Render::DrawParams params = DRAW_PARAMS_CONTENT;
        params.material = atlas_->material();

        struct SpriteDemo { float x; glm::vec4 tint; };
        static constexpr std::array<SpriteDemo, 3> DEMOS = {{
            { -48.0f, Palette::White                      }, // untinted
            {   0.0f, Palette::Lavender                   }, // tinted
            {  48.0f, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.4f } }, // translucent
        }};

        for (const SpriteDemo& d : DEMOS) {
            r->draw_sprite(
                Render::Sprite{
                    .pos_box = { d.x - 16.0f, -192.0f, d.x + 16.0f, -128.0f },
                    .tex_box = tex_box,
                    .tint    = d.tint,
                },
                params
            );
        }
    }

    // -------------------------------------------------------------------------
    // Draw SDF Texts
    //
    // Outline, drop shadow (offset outline), bold, thin, and centered
    // multi-line text.
    // -------------------------------------------------------------------------

    void draw_sdf_texts_(Render::Renderer* r) {
        static constexpr float LEFT_X  = 195.0f;
        static constexpr float RIGHT_X = 310.0f;

        draw_subtitle_(r, "SDF Text", { 250.0f, -250.0f });

        // Bold
        r->draw_text(
            "Bold", { LEFT_X, -112.0f }, *font_,
            Render::TextStyle {
                .size   = 22.0f,
                .color  = Palette::White,
                .weight = 1.6f,
                .align  = Render::TextAlign::Center,
                .valign = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );

        // Thin
        r->draw_text(
            "Thin", { RIGHT_X, -112.0f }, *font_,
            Render::TextStyle {
                .size   = 22.0f,
                .color  = Palette::White,
                .weight = -0.6f,
                .align  = Render::TextAlign::Center,
                .valign = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );

        // Outline
        r->draw_text(
            "Outline", { LEFT_X, -150.0f }, *font_,
            Render::TextStyle {
                .size          = 22.0f,
                .color         = Palette::Lavender,
                .outline_color = Palette::Magenta,
                .outline_width = 3.0f,
                .align         = Render::TextAlign::Center,
                .valign        = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );

        // Drop shadow: soft, offset outline down-right
        r->draw_text(
            "Shadow", { RIGHT_X, -150.0f }, *font_,
            Render::TextStyle {
                .size           = 22.0f,
                .color          = Palette::Cyan,
                .outline_color  = { 0.0f, 0.0f, 0.0f, 0.8f },
                .outline_width  = 2.0f,
                .outline_offset = { 4.0f, -4.0f },
                .align          = Render::TextAlign::Center,
                .valign         = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );

        // Centered multi-line
        r->draw_text(
            "Centered\nmulti-line", { 250.0f, -198.0f }, *font_,
            Render::TextStyle {
                .size          = 18.0f,
                .color         = Palette::Yellow,
                .outline_color = Palette::Black,
                .outline_width = 3.0f,
                .line_spacing  = 1.1f,
                .align         = Render::TextAlign::Center,
                .valign        = Render::TextVAlign::Middle,
            },
            DRAW_PARAMS_CONTENT
        );
    }

    // -------------------------------------------------------------------------
    // Draw Stats
    //
    // Frame statistics in the top-left corner, drawn through the screen view
    // so they stay put when the world camera moves. Renderer stats are for
    // the previous frame.
    // -------------------------------------------------------------------------

    void draw_stats_(
        Render::Renderer*                   r,
        const Engine::LoopStats&            e_stats,
        const Render::Renderer::FrameStats& r_stats
    ) {
        char text[160];
        std::snprintf(text, sizeof text,
            "draw %.2f ms\n"
            "objects %u\n"
            "draw calls %u",
            e_stats.draw_ms,
            r_stats.objects,
            r_stats.draw_calls
        );

        const glm::vec2 top_left = { 8.0f, screen_camera_->get_size().y - 8.0f };

        r->draw_text(
            text,
            top_left,
            *font_,
            Render::TextStyle {
                .size          = 16.0f,
                .color         = Palette::White,
                .outline_color = Palette::Black,
                .outline_width = 3.0f,
                .align         = Render::TextAlign::Left,
                .valign        = Render::TextVAlign::Top,
            },
            DRAW_PARAMS_UI
        );
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    // Owned by the renderer. SlotMap objects never move, so these stay valid
    // until removed, and this example never removes them.
    Render::Camera* world_camera_  = nullptr;
    Render::Camera* screen_camera_ = nullptr;
    Render::Atlas*  atlas_         = nullptr;
    Render::Font*   font_          = nullptr;
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        EngineConfig config = { .escape_exits = true };
        Engine engine(config);
        RendererExample app;
        engine.run(app);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
