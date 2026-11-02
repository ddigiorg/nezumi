// =============================================================================
// Particle.hpp
//
// One cosmetic particle (48 bytes): rain, snow, dust, sparks, smoke. Lives in
// a ParticlePool. All positions and speeds are in VIEW space (what the world
// camera looks at), so the engine stays out of how a game projects its
// world: a game with a projection projects before it spawns.
//
// Particles never affect game state. They are not deterministic and are
// never saved, so update them on frame time and spawn them from whatever
// RNG you like (Factorio's rule for its "simple" particles).
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint>     // uint32_t
#include <glm/glm.hpp> // vec2, vec4

namespace Nezumi::Render {

enum class ParticleFlags : std::uint32_t {
    None    = 0,
    Gravity = 1 << 0, // falls with ParticlePoolConfig::gravity
    FadeOut = 1 << 1, // alpha goes to 0 over its lifetime
    Shrink  = 1 << 2, // size goes to 0 over its lifetime
    Streak  = 1 << 3, // drawn as a line trailing its velocity (rain, sparks)
};

ENUM_CLASS_FLAGS(ParticleFlags);

struct Particle {
    glm::vec2     position = { 0.0f, 0.0f };       // view units
    glm::vec2     velocity = { 0.0f, 0.0f };       // view units per second
    glm::vec4     color    = { 1.0f, 1.0f, 1.0f, 1.0f };
    float         size     = 0.1f;                 // side (square) or width (streak), view units
    float         age      = 0.0f;                 // seconds
    float         lifetime = 1.0f;                 // seconds; dead once age reaches it
    ParticleFlags flags    = ParticleFlags::None;

    bool  is_dead()  const { return age >= lifetime; }
    float progress() const { return lifetime > 0.0f ? age / lifetime : 1.0f; } // 0 born, 1 dead
};

static_assert(sizeof(Particle) == 48, "Particle is 48 bytes");

} // namespace Nezumi::Render
