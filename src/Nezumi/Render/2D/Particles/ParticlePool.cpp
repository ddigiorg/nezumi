// =============================================================================
// ParticlePool.cpp
// =============================================================================

#include "Nezumi/Render/2D/Particles/ParticlePool.hpp"
#include "Nezumi/Render/2D/Renderer2D.hpp"
#include <algorithm> // max

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

ParticlePool::ParticlePool(const ParticlePoolConfig& config)
    : config_   (config)
    , particles_(config.capacity)
{}

// -----------------------------------------------------------------------------
// Particles
// -----------------------------------------------------------------------------

void ParticlePool::update(float dt) {
    const float keep = std::max(0.0f, 1.0f - config_.drag * dt);

    live_ = 0;
    particles_.for_each([&](Particle& p) {
        if (p.is_dead()) return;

        p.age += dt;
        if (p.is_dead()) return;

        if (has_any(p.flags, ParticleFlags::Gravity)) p.velocity += config_.gravity * dt;
        p.velocity *= keep;
        p.position += p.velocity * dt;
        live_++;
    });

    // Lifetimes differ, so dead particles can sit behind live ones; trimming
    // the oldest end is enough to keep the walk short in steady state
    while (!particles_.empty() && particles_.front().is_dead()) particles_.pop_front();
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void ParticlePool::draw(Renderer2D& renderer, const DrawParams& params, glm::vec4 view_box) const {
    particles_.for_each([&](const Particle& p) {
        if (p.is_dead()) return;

        const float t = p.progress();

        glm::vec4 color = p.color;
        if (has_any(p.flags, ParticleFlags::FadeOut)) color.a *= 1.0f - t;

        float size = p.size;
        if (has_any(p.flags, ParticleFlags::Shrink)) size *= 1.0f - t;

        if (has_any(p.flags, ParticleFlags::Streak)) {
            const glm::vec2 tail = p.position - p.velocity * config_.streak_time;

            const float min_x = std::min(tail.x, p.position.x), max_x = std::max(tail.x, p.position.x);
            const float min_y = std::min(tail.y, p.position.y), max_y = std::max(tail.y, p.position.y);
            if (max_x < view_box.x || min_x > view_box.z || max_y < view_box.y || min_y > view_box.w) return;

            renderer.draw_line(
                Line{ .position_a = tail, .position_b = p.position, .color = color, .width = size },
                params);
            return;
        }

        const float h = 0.5f * size;
        const glm::vec4 box = { p.position.x - h, p.position.y - h, p.position.x + h, p.position.y + h };
        if (box.z < view_box.x || box.x > view_box.z || box.w < view_box.y || box.y > view_box.w) return;

        renderer.draw_rectangle(Rectangle{ .pos_box = box, .color = color }, params);
    });
}

} // namespace Nezumi::Render
