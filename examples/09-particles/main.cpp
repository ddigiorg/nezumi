// =============================================================================
// examples/09-particles/main.cpp
//
// Three ParticlePools, each fed by a tiny emitter that lives here, in the
// app, the way a game would own its own:
//
// - Rain: streaks spawned across the top of whatever the camera sees, so it
//   covers the screen at any pan or zoom. A pool of its own, so the bursts
//   below can never overwrite it.
// - Sparks: a fountain at the origin. Gravity pulls them down; they fade.
// - Dust: a puff at a random spot every half second. Drag slows it, and it
//   shrinks and fades.
//
// W/A/S/D pans, the mouse wheel zooms. Escape quits.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Engine.hpp"
#include "Nezumi/Render/2D/Particles/ParticlePool.hpp"
#include <cmath>       // cos, sin
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <numbers>     // pi_v

using namespace Nezumi;
using Render::Particle;
using Render::ParticleFlags;
using Render::ParticlePool;

namespace Action {
    inline constexpr ActionID PanLeft  = 0;
    inline constexpr ActionID PanRight = 1;
    inline constexpr ActionID PanDown  = 2;
    inline constexpr ActionID PanUp    = 3;
}

// Emission rates, per second
static constexpr float RAIN_RATE  = 600.0f;
static constexpr float SPARK_RATE = 240.0f;
static constexpr float DUST_EVERY = 0.5f; // seconds between puffs

class ParticlesExample final : public IApplication {
public:

    void on_init(Engine& engine) override {
        Render::Renderer2D& r = *engine.renderer2d();

        InputMap& input = engine.input();
        input.bind(Action::PanLeft,  { ScanCode::A, ScanCode::Left  });
        input.bind(Action::PanRight, { ScanCode::D, ScanCode::Right });
        input.bind(Action::PanDown,  { ScanCode::S, ScanCode::Down  });
        input.bind(Action::PanUp,    { ScanCode::W, ScanCode::Up    });

        // One world unit is 32 px at zoom 1, so the sizes below read as tiles
        camera_ = r.get_view_camera(Render::View::World);
        camera_->set_pixels_per_unit(32.0f);
        camera_->set_position({ 0.0f, 3.0f });
        camera_controller_ = std::make_unique<CameraController>(
            engine.event_bus(), input, r, r.get_view(Render::View::World),
            CameraControllerConfig{
                .zoom_min  = 0.25f,
                .zoom_max  = 4.0f,
                .pan_left  = Action::PanLeft,
                .pan_right = Action::PanRight,
                .pan_down  = Action::PanDown,
                .pan_up    = Action::PanUp,
            });

        engine.logger().info("Particles example: W/A/S/D pan, wheel zoom, Escape quits");
    }

    void on_tick(Engine& engine) override {
        // Particles are cosmetic: frame time would do as well. The fixed
        // step keeps this example's rates simple.
        const float dt  = engine.clock().get_fixed_dt();
        Random&     rng = engine.random();

        emit_rain_ (rng, dt);
        emit_sparks_(rng, dt);
        emit_dust_ (rng, dt);

        rain_  .update(dt);
        sparks_.update(dt);
        dust_  .update(dt);
    }

    void on_draw(Engine& engine) override {
        Render::Renderer2D& r    = *engine.renderer2d();
        const glm::vec4     view = camera_->world_bounds();

        // Ground, so the fade and the rain read against something
        r.draw_rectangle(Render::Rectangle{ .pos_box = { -100.0f, -100.0f, 100.0f, 0.0f },
                                            .color   = { 0.16f, 0.20f, 0.14f, 1.0f } },
                         { .view = Render::View::World, .layer = 0 });

        dust_  .draw(r, { .view = Render::View::World, .layer = 1 }, view);
        sparks_.draw(r, { .view = Render::View::World, .layer = 2 }, view);
        rain_  .draw(r, { .view = Render::View::World, .layer = 3 }, view);
    }

    void on_exit(Engine& engine) override {
        engine.logger().info("Particles alive at exit: rain " + std::to_string(rain_.count()) +
                             ", sparks " + std::to_string(sparks_.count()) +
                             ", dust " + std::to_string(dust_.count()));
    }

private:

    // A few per tick across the top of the view (plus a margin for the
    // slant), falling fast and slightly sideways
    void emit_rain_(Random& rng, float dt) {
        const glm::vec4 view  = camera_->world_bounds();
        const float     width = view.z - view.x;
        const float     scale = width / 40.0f; // keep the look at any zoom

        rain_carry_ += RAIN_RATE * dt;
        for (; rain_carry_ >= 1.0f; rain_carry_ -= 1.0f) {
            rain_.spawn(Particle{
                .position = { rng.get<float>(view.x - 0.2f * width, view.z), view.w },
                .velocity = glm::vec2{ 6.0f, -40.0f } * scale,
                .color    = { 0.65f, 0.75f, 0.95f, 0.55f },
                .size     = 0.04f * scale,
                .lifetime = rng.get<float>(0.4f, 1.2f) * (view.w - view.y) / (40.0f * scale),
                .flags    = ParticleFlags::Streak,
            });
        }
    }

    // A cone upward from the origin
    void emit_sparks_(Random& rng, float dt) {
        spark_carry_ += SPARK_RATE * dt;
        for (; spark_carry_ >= 1.0f; spark_carry_ -= 1.0f) {
            const float angle = std::numbers::pi_v<float> * rng.get<float>(0.35f, 0.65f);
            const float speed = rng.get<float>(6.0f, 11.0f);
            sparks_.spawn(Particle{
                .position = { 0.0f, 0.0f },
                .velocity = glm::vec2{ std::cos(angle), std::sin(angle) } * speed,
                .color    = { 1.0f, rng.get<float>(0.45f, 0.8f), 0.15f, 1.0f },
                .size     = 0.06f,
                .lifetime = rng.get<float>(0.8f, 1.6f),
                .flags    = ParticleFlags::Gravity | ParticleFlags::FadeOut | ParticleFlags::Streak,
            });
        }
    }

    // 24 grains bursting out of a random point on the ground
    void emit_dust_(Random& rng, float dt) {
        dust_timer_ -= dt;
        if (dust_timer_ > 0.0f) return;
        dust_timer_ += DUST_EVERY;

        const glm::vec2 at = { rng.get<float>(-10.0f, 10.0f), rng.get<float>(-6.0f, -1.0f) };
        for (int i = 0; i < 24; i++) {
            const float angle = 2.0f * std::numbers::pi_v<float> * rng.get<float>(0.0f, 1.0f);
            const float speed = rng.get<float>(0.5f, 2.5f);
            dust_.spawn(Particle{
                .position = at,
                .velocity = glm::vec2{ std::cos(angle), 0.5f * std::sin(angle) + 0.6f } * speed,
                .color    = { 0.62f, 0.55f, 0.42f, 0.8f },
                .size     = rng.get<float>(0.15f, 0.35f),
                .lifetime = rng.get<float>(0.6f, 1.2f),
                .flags    = ParticleFlags::FadeOut | ParticleFlags::Shrink,
            });
        }
    }

    Render::Camera2D*                 camera_ = nullptr;
    std::unique_ptr<CameraController> camera_controller_;

    ParticlePool rain_  { { .capacity = 2048, .gravity = { 0.0f, 0.0f } } };
    ParticlePool sparks_{ { .capacity = 1024, .gravity = { 0.0f, -12.0f } } };
    ParticlePool dust_  { { .capacity = 512,  .gravity = { 0.0f, 0.0f }, .drag = 3.0f } };

    float rain_carry_  = 0.0f;
    float spark_carry_ = 0.0f;
    float dust_timer_  = 0.0f;
};

int main() {
    try {
        EngineConfig config = {};
        config.window.title = "Particles";
        config.escape_exits = true;

        Engine engine(config);
        ParticlesExample app;
        engine.run(app);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
