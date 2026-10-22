// =============================================================================
// Unit.cpp
// =============================================================================

#include "Nezumi/World/Entities/Unit.hpp"
#include "Nezumi/World/Entity/EntitySearchFilter.hpp"
#include "Nezumi/World/World.hpp"
#include "Nezumi/World/WorldContext.hpp"
#include <cassert> // assert
#include <cmath>   // sqrt
#include <cstdint> // UINT16_MAX

namespace Nezumi {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Unit::Unit(const UnitConfig& config, EntityFactionsMask factions)
    : EntityDamageable(config.max_health)
    , config_(&config)
{
    this->set_active();
    this->set_tickable();
    this->set_drawable();

    subtype_id       = config.subtype;
    factions_mask    = factions;
    collision_bounds = config.collision_bounds;
}

// -----------------------------------------------------------------------------
// Tick
// -----------------------------------------------------------------------------

void Unit::tick(WorldContext& ctx) {
    if (attack_cooldown_ > 0) attack_cooldown_--;
    velocity = {};

    const UnitState prev_state = state_;

    const float aggro = config_->aggro_radius;

    switch (command_.type) {
        case UnitCommandType::Stop: {
            const Entity* target = resolve_target_(ctx, aggro);
            if (!target) target = acquire_target_(ctx);

            if (target) engage_(ctx, *target);
            else        state_ = UnitState::Idle;
            break;
        }

        case UnitCommandType::MoveTo: {
            state_ = UnitState::Moving;
            if (move_toward_(ctx, command_.position, 0.0f)) stop();
            break;
        }

        case UnitCommandType::Attack: {
            const Entity* target = resolve_target_(ctx, 0.0f);

            if (target) engage_(ctx, *target);
            else        stop();
            break;
        }

        case UnitCommandType::AttackMove: {
            const Entity* target = resolve_target_(ctx, aggro);
            if (!target) target = acquire_target_(ctx);

            if (target) {
                engage_(ctx, *target);
            } else {
                state_ = UnitState::Moving;
                if (move_toward_(ctx, command_.position, 0.0f)) stop();
            }
            break;
        }
    }

    // A command that completed this tick leaves the unit standing still
    if (velocity.is_zero() && state_ == UnitState::Moving) state_ = UnitState::Idle;

    if (state_ != prev_state)  state_ticks_ = 0;
    else if (state_ticks_ < UINT16_MAX) state_ticks_++;
}

// -----------------------------------------------------------------------------
// Behaviors
// -----------------------------------------------------------------------------

const Entity* Unit::resolve_target_(WorldContext& ctx, float leash) {
    if (!target_) return nullptr;

    const Entity* target = ctx.world->get_entity(target_);
    const bool lost = !target || !target->is_alive() ||
                      (leash > 0.0f && distance2(*target) > leash * leash);

    if (lost) {
        target_ = {};
        return nullptr;
    }
    return target;
}

const Entity* Unit::acquire_target_(WorldContext& ctx) {
    const float aggro = config_->aggro_radius;
    if (aggro <= 0.0f) return nullptr;
    if ((ctx.tick + handle.index()) % ACQUIRE_INTERVAL != 0) return nullptr;

    const Surface* surface = ctx.world->get_surface(surface_id);
    assert(surface && "Unit is not on a valid surface");

    target_ = surface->find_nearest_entity(position, aggro, EntitySearchFilter::enemies_of(*this));
    return target_ ? ctx.world->get_entity(target_) : nullptr;
}

void Unit::engage_(WorldContext& ctx, const Entity& target) {
    const float range = config_->attack_range;

    if (!move_toward_(ctx, target.position, range)) {
        state_ = UnitState::Moving;
        return;
    }

    // In range: turn to the target and hit it whenever off cooldown
    const Position2D delta = target.position - position;
    face_(delta, std::sqrt(delta.x * delta.x + delta.y * delta.y));
    state_ = UnitState::Attacking;

    if (attack_cooldown_ == 0) {
        ctx.world->damage_entity(target.handle, config_->attack_damage, handle);
        attack_cooldown_ = config_->attack_interval;
    }
}

bool Unit::move_toward_(WorldContext& ctx, Position2D destination, float stop_distance) {
    const Position2D delta = destination - position;
    const float      dist2 = delta.x * delta.x + delta.y * delta.y;
    const float      stop  = stop_distance > ARRIVE_DISTANCE ? stop_distance : ARRIVE_DISTANCE;

    if (dist2 <= stop * stop) {
        velocity = {};
        return true;
    }

    const float dist   = std::sqrt(dist2);
    const float travel = dist - stop_distance;       // distance left to cover
    const float step   = config_->move_speed * ctx.dt;

    face_(delta, dist);

    // Don't overshoot: on the last step, move exactly the remaining distance
    const float speed = travel < step ? travel / ctx.dt : config_->move_speed;
    velocity = facing_ * speed;
    return false;
}

void Unit::face_(Position2D delta, float distance) {
    if (distance > 0.0f) facing_ = delta / distance;
}

} // namespace Nezumi
