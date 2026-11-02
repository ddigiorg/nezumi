// =============================================================================
// ParticlePool.hpp
//
// A fixed-capacity pool of cosmetic particles (Particle.hpp), updated on the
// CPU and drawn through Renderer2D as squares or streaks.
//
//   ParticlePool rain({ .capacity = 4096, .gravity = { 0.0f, -30.0f } });
//
//   // anywhere (an emitter, an impact):
//   rain.spawn({ .position = p, .velocity = v, .color = c, .size = 0.05f,
//                .lifetime = 1.0f, .flags = ParticleFlags::Streak });
//
//   // once per frame:
//   rain.update(dt);
//   rain.draw(renderer, { .view = View::World, .layer = 3 }, camera.world_bounds());
//
// A full pool overwrites its oldest particle instead of allocating, so a
// burst can never grow memory; give long-lived effects (weather) a pool of
// their own so short bursts don't eat them. Emitters (what spawns where and
// how often) belong to the game, which knows its biomes, weapons and camera.
//
// Not thread-safe.
// =============================================================================

#pragma once
#include "Nezumi/Object/RingBuffer.hpp"
#include "Nezumi/Render/2D/DrawParams.hpp"
#include "Nezumi/Render/2D/Particles/Particle.hpp"
#include <cstddef>     // size_t
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

class Renderer2D;

struct ParticlePoolConfig {
    std::size_t capacity    = 4096;
    glm::vec2   gravity     = { 0.0f, -9.8f }; // view units per second^2, for ParticleFlags::Gravity
    float       drag        = 0.0f;            // fraction of velocity lost per second
    float       streak_time = 0.04f;           // a streak trails velocity * this (seconds)
};

class ParticlePool {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit ParticlePool(const ParticlePoolConfig& config = {});

    // -------------------------------------------------------------------------
    // Particles
    // -------------------------------------------------------------------------

    // Add a particle; overwrites the oldest one when the pool is full
    void spawn(const Particle& particle) { particles_.push(particle); }

    // Age, move and expire every particle by `dt` seconds
    void update(float dt);

    // Remove every particle
    void clear() { particles_.clear(); }

    // -------------------------------------------------------------------------
    // Draw
    //
    // Submits each live particle that overlaps `view_box` ({ min_x, min_y,
    // max_x, max_y }, view units): squares as rectangles, streaks as lines,
    // all with `params`.
    // -------------------------------------------------------------------------

    void draw(Renderer2D& renderer, const DrawParams& params, glm::vec4 view_box) const;

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    const ParticlePoolConfig& config()   const { return config_;                 }
    std::size_t               capacity() const { return particles_.capacity();   }

    // Live particles after the last update (or slots in use, between an
    // update and the next spawns)
    std::size_t               count()    const { return live_;                   }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        particles_.for_each([&](const Particle& p) { if (!p.is_dead()) fn(p); });
    }

private:
    ParticlePoolConfig   config_;
    RingBuffer<Particle> particles_;
    std::size_t          live_ = 0;
};

} // namespace Nezumi::Render
