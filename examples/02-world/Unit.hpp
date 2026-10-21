// =============================================================================
// Unit.hpp
//
// A unit that wanders around at walking speed. All sizes and speeds are in
// tiles (see World/Constants.hpp), never pixels.
// =============================================================================

#pragma once
#include "Nezumi/Core/Random.hpp"
#include "Nezumi/World/Entity/Entity.hpp"
#include "Nezumi/World/WorldContext.hpp"
#include <cstdint> // uint8_t

using namespace Nezumi;

class Unit : public Nezumi::Entity {
public:
    static constexpr EntityTypeID TYPE_ID{ 1 };

    static constexpr float SPEED     = 2.0f;  // tiles per second
    static constexpr int   TURN_ODDS = 60;    // 1 in N ticks picks a new heading (~1 s)

    explicit Unit(uint8_t team) : team(team) {
        this->set_active();
        this->set_tickable();

        // Half a tile square, centered on the unit's feet
        collision_bounds = { -0.25f, -0.25f, 0.25f, 0.25f };
    };

    void tick(WorldContext& ctx) override {
        if (started_ && ctx.random->get<int>(0, TURN_ODDS - 1) != 0) return;
        started_ = true;

        // One of 8 directions, or standing still. Only velocity is written
        // here: the movement phase moves the unit and updates its chunk.
        Position2D dir = {
            static_cast<float>(ctx.random->get<int>(-1, 1)),
            static_cast<float>(ctx.random->get<int>(-1, 1)),
        };
        if (dir.x != 0.0f && dir.y != 0.0f) dir *= 0.70710678f; // diagonals not faster
        velocity = dir * SPEED;
    }

    uint8_t team = 0;

private:
    bool started_ = false;
};
