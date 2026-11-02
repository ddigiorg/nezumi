// =============================================================================
// examples/07-lighting/main.cpp
//
// 2D lighting: a world drawn into a scene target, lights added into a
// half-size light target, then PostProcess composites them (scene x (ambient
// + light)), adds bloom, and tonemaps onto the screen. The UI draws last,
// unlit.
//
// - A field of colored tiles and sprites, lit by:
//     a flickering torch with an unlit flame (View::Emissive),
//     three colored lights orbiting the center, bright enough to bloom,
//     a light that follows the mouse.
// - The ambient color slides from day to a near-black night.
//
// Controls: W/A/S/D or arrows pan, mouse wheel zooms. Up/Down: time of day.
// 1: lighting on/off. 2: bloom on/off. 3: debug view (none, light, bloom).
// 4: tonemap (clamp, shoulder, ACES). Escape quits.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Engine.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Render/Core/Post/PostProcess.hpp"
#include <array>       // array
#include <cmath>       // sin, cos
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <glm/glm.hpp> // vec2, vec3, vec4, mix
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique

using namespace Nezumi;

namespace Action {
    inline constexpr ActionID PanLeft       = 0;
    inline constexpr ActionID PanRight      = 1;
    inline constexpr ActionID PanDown       = 2;
    inline constexpr ActionID PanUp         = 3;
    inline constexpr ActionID Later         = 4;
    inline constexpr ActionID Earlier       = 5;
    inline constexpr ActionID ToggleLights  = 6;
    inline constexpr ActionID ToggleBloom   = 7;
    inline constexpr ActionID CycleDebug    = 8;
    inline constexpr ActionID CycleTonemap  = 9;
}

// Ambient light (linear) from noon (t = 0) to midnight (t = 1)
static glm::vec3 ambient_at(float t) {
    const glm::vec3 day   = { 1.00f, 1.00f, 1.00f };
    const glm::vec3 dusk  = { 0.55f, 0.32f, 0.22f };
    const glm::vec3 night = { 0.020f, 0.026f, 0.055f };
    return t < 0.5f ? glm::mix(day, dusk, t * 2.0f) : glm::mix(dusk, night, (t - 0.5f) * 2.0f);
}

// Cheap 1D value noise for flicker, 0..1
static float noise1(float x) {
    const auto hash = [](int i) {
        unsigned n = static_cast<unsigned>(i) * 374761393u + 668265263u;
        n = (n ^ (n >> 13)) * 1274126177u;
        return float(n & 0xFFFFu) / 65535.0f;
    };
    const int   i = static_cast<int>(std::floor(x));
    const float f = x - float(i);
    const float s = f * f * (3.0f - 2.0f * f);
    return hash(i) + (hash(i + 1) - hash(i)) * s;
}

class LightingExample final : public IApplication {
public:

    void on_init(Engine& engine) override {
        Render::RenderCore& core = *engine.render();
        Render::Renderer2D& r    = *engine.renderer2d();

        // Targets: the world at full size, the light at half (lights are soft)
        const Render::TargetID scene = core.create_target({ .name = "scene" });
        const Render::TargetID light = core.create_target({ .name = "light", .scale = 0.5f });

        post_ = std::make_unique<Render::PostProcess>(core, Render::PostProcessConfig{
            .scene = scene,
            .light = light,
        });

        // The lit recipe, replacing the Engine's single pass
        using Render::view_bit;
        using Render::View;
        Render::FrameRecipe recipe;
        recipe.passes.push_back({ .name = "scene", .target = scene,
                                  .clear_color = { 0.05f, 0.05f, 0.06f, 1.0f },
                                  .features = { { &r, view_bit(View::World) } } });
        recipe.passes.push_back({ .name = "lights", .target = light,
                                  .clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
                                  .features = { { &r, view_bit(View::Lights) } } });
        recipe.passes.push_back(post_->composite_pass());
        recipe.passes.push_back({ .name = "emissive", .target = post_->lit_target(), .clear = false,
                                  .features = { { &r, view_bit(View::Emissive) } } });
        for (Render::PassConfig& p : post_->output_passes()) recipe.passes.push_back(std::move(p));
        recipe.passes.push_back({ .name = "ui", .clear = false,
                                  .features = { { &r, view_bit(View::Screen) } } });
        core.set_recipe(std::move(recipe));

        // Camera and input
        InputMap& input = engine.input();
        input.bind(Action::PanLeft,      { ScanCode::A, ScanCode::Left  });
        input.bind(Action::PanRight,     { ScanCode::D, ScanCode::Right });
        input.bind(Action::PanDown,      { ScanCode::S });
        input.bind(Action::PanUp,        { ScanCode::W });
        input.bind(Action::Later,        ScanCode::Down);
        input.bind(Action::Earlier,      ScanCode::Up);
        input.bind(Action::ToggleLights, ScanCode::Num1);
        input.bind(Action::ToggleBloom,  ScanCode::Num2);
        input.bind(Action::CycleDebug,   ScanCode::Num3);
        input.bind(Action::CycleTonemap, ScanCode::Num4);

        world_camera_  = r.get_view_camera(View::World);
        screen_camera_ = r.get_view_camera(View::Screen);
        camera_controller_ = std::make_unique<CameraController>(
            engine.event_bus(), input, r, r.get_view(View::World),
            CameraControllerConfig{
                .zoom_min  = 0.5f,
                .zoom_max  = 4.0f,
                .pan_left  = Action::PanLeft,
                .pan_right = Action::PanRight,
                .pan_down  = Action::PanDown,
                .pan_up    = Action::PanUp,
            });

        atlas_ = r.create_atlas({ .name = "example-atlas", .filepath = "assets/graphics/atlas.png" }).object;
        font_  = r.create_font({ .name = "example-font",
                                 .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf" }).object;

        mouse_listener_ = engine.event_bus().subscribe<OnMouseMotion>([this](const OnMouseMotion& e) {
            mouse_ = { e.pos_x, e.pos_y };
        });

        engine.logger().info("Lighting example: Up/Down time of day, 1 lighting, 2 bloom, 3 debug view, 4 tonemap");
    }

    void on_tick(Engine& engine) override {
        InputMap& input = engine.input();
        Render::PostSettings& s = post_->settings();

        if (input.is_held(Action::Later))   time_ = std::min(time_ + 0.01f, 1.0f);
        if (input.is_held(Action::Earlier)) time_ = std::max(time_ - 0.01f, 0.0f);
        if (input.was_pressed(Action::ToggleLights)) s.lighting = !s.lighting;
        if (input.was_pressed(Action::ToggleBloom))  s.bloom    = !s.bloom;
        if (input.was_pressed(Action::CycleDebug))
            s.debug = static_cast<Render::PostDebugView>((static_cast<int>(s.debug) + 1) % 3);
        if (input.was_pressed(Action::CycleTonemap))
            s.tonemap = static_cast<Render::Tonemap>((static_cast<int>(s.tonemap) + 1) % 3);
    }

    void on_draw(Engine& engine) override {
        Render::Renderer2D& r = *engine.renderer2d();
        camera_controller_->update(engine.clock().get_real_dt());

        seconds_ += engine.clock().get_real_dt();
        post_->settings().ambient = ambient_at(time_);

        draw_ground_(r);
        draw_sprites_(r);
        draw_lights_(r);
        draw_hud_(r);
    }

    void on_exit(Engine& engine) override {
        engine.event_bus().unsubscribe(mouse_listener_);
        camera_controller_.reset();
        post_.reset();
    }

private:

    // A 24 x 16 field of tiles in muted colors
    void draw_ground_(Render::Renderer2D& r) const {
        static constexpr std::array<glm::vec4, 4> COLORS = {{
            { 0.36f, 0.42f, 0.26f, 1.0f },
            { 0.40f, 0.46f, 0.30f, 1.0f },
            { 0.33f, 0.38f, 0.30f, 1.0f },
            { 0.44f, 0.40f, 0.32f, 1.0f },
        }};
        for (int y = -8; y < 8; y++) {
            for (int x = -12; x < 12; x++) {
                const float fx = float(x) * 32.0f, fy = float(y) * 32.0f;
                r.draw_rectangle({ .pos_box = { fx, fy, fx + 32.0f, fy + 32.0f },
                                   .color = COLORS[std::size_t((x * 7 + y * 13) & 3)] },
                                 { .view = Render::View::World, .layer = 0 });
            }
        }
    }

    // A ring of the atlas's arrow sprites
    void draw_sprites_(Render::Renderer2D& r) const {
        for (int i = 0; i < 20; i++) {
            const float     a  = float(i) / 20.0f * 6.2831853f;
            const glm::vec2 p  = { std::cos(a) * 220.0f, std::sin(a) * 150.0f };
            const glm::vec4 px = { float(i % 10) * 64.0f, float(i / 10) * 128.0f,
                                   float(i % 10) * 64.0f + 64.0f, float(i / 10) * 128.0f + 128.0f };
            r.draw_sprite({ .pos_box = { p.x - 12.0f, p.y, p.x + 12.0f, p.y + 48.0f },
                            .tex_box = atlas_->tex_box(px) },
                          { .view = Render::View::World, .layer = 1, .order = -int(p.y),
                            .material = atlas_->material() });
        }
    }

    void draw_lights_(Render::Renderer2D& r) const {
        using Render::PointLight;

        // Torch: warm, flickering, with a small hot flame on the emissive layer
        const glm::vec2 torch   = { -120.0f, 40.0f };
        const float     flicker = 0.9f + 0.2f * noise1(seconds_ * 9.0f) * noise1(seconds_ * 3.1f + 7.0f);
        r.draw_light(PointLight::circle(torch, 190.0f * (0.97f + 0.03f * flicker),
                                        { 1.0f, 0.55f, 0.25f, 2.2f * flicker }, { 10.0f, 0.0f, 0.0f, 0.0f }));
        r.draw_rectangle({ .pos_box = { torch.x - 2.0f, torch.y - 26.0f, torch.x + 2.0f, torch.y },
                           .color = { 0.25f, 0.16f, 0.10f, 1.0f } },
                         { .view = Render::View::World, .layer = 1 });
        r.draw_light(PointLight::circle(torch + glm::vec2(0.0f, 4.0f), 9.0f,
                                        { 1.0f, 0.65f, 0.3f, 12.0f * flicker }, { 2.0f, 0.3f, 0.0f, 0.0f }),
                     { .view = Render::View::Emissive });

        // Three colored lights orbiting the center: bright, so they bloom
        static constexpr std::array<glm::vec3, 3> MAGIC = {{
            { 0.25f, 0.55f, 1.00f }, // frost
            { 0.70f, 0.30f, 1.00f }, // arcane
            { 0.30f, 1.00f, 0.45f }, // poison
        }};
        for (std::size_t i = 0; i < MAGIC.size(); i++) {
            const float     a     = seconds_ * 0.6f + float(i) * 2.0944f;
            const glm::vec2 p     = { 150.0f * std::cos(a), 100.0f * std::sin(a) };
            const float     pulse = 1.0f + 0.25f * std::sin(seconds_ * 4.0f + float(i));
            r.draw_light(PointLight::circle(p, 140.0f, { MAGIC[i], 1.6f * pulse }, { 4.0f, 0.4f, 0.0f, 0.0f }));
            r.draw_light(PointLight::circle(p, 7.0f, { MAGIC[i], 14.0f * pulse }, { 1.0f, 0.5f, 0.3f, 0.0f }),
                         { .view = Render::View::Emissive });
        }

        // The mouse carries a soft white light
        if (mouse_.x >= 0.0f) {
            const glm::vec2 at = world_camera_->screen_to_world(mouse_);
            r.draw_light(PointLight::circle(at, 110.0f, { 1.0f, 0.95f, 0.85f, 1.2f }, { 6.0f, 0.2f, 0.0f, 0.0f }));
        }
    }

    void draw_hud_(Render::Renderer2D& r) const {
        static constexpr const char* DEBUG[]   = { "none", "light", "bloom" };
        static constexpr const char* TONEMAP[] = { "clamp", "shoulder", "aces" };
        const Render::PostSettings& s = post_->settings();

        char text[256];
        std::snprintf(text, sizeof text,
            "time %3.0f%% (up/down)\n1 lighting %s\n2 bloom    %s\n3 debug    %s\n4 tonemap  %s",
            time_ * 100.0f, s.lighting ? "on" : "off", s.bloom ? "on" : "off",
            DEBUG[static_cast<int>(s.debug)], TONEMAP[static_cast<int>(s.tonemap)]);

        const float top = screen_camera_->get_size().y / screen_camera_->get_scale() - 8.0f;
        r.draw_text(text, { 8.0f, top }, *font_,
                    Render::TextStyle{
                        .size          = 16.0f,
                        .color         = { 1.0f, 1.0f, 1.0f, 1.0f },
                        .outline_color = { 0.0f, 0.0f, 0.0f, 1.0f },
                        .outline_width = 3.0f,
                        .valign        = Render::TextVAlign::Top,
                    },
                    { .view = Render::View::Screen });
    }

    std::unique_ptr<Render::PostProcess> post_              = nullptr;
    std::unique_ptr<CameraController>    camera_controller_ = nullptr;
    Render::Camera2D* world_camera_  = nullptr;
    Render::Camera2D* screen_camera_ = nullptr;
    Render::Atlas*    atlas_         = nullptr;
    Render::Font*     font_          = nullptr;
    EventListenerID   mouse_listener_ = {};
    glm::vec2         mouse_          = { -1.0f, -1.0f };
    float             time_           = 0.85f; // mostly night
    float             seconds_        = 0.0f;
};

int main() {
    try {
        EngineConfig config = { .escape_exits = true };
        config.window.title = "Nezumi: 07-lighting";
        Engine engine(config);
        LightingExample app;
        engine.run(app);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
