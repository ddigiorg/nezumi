// =============================================================================
// UnitCommand.hpp
//
// What a unit was told to do. A command persists across ticks until it
// completes or is replaced; the unit's UnitState is what it does about it
// this tick. A future UnitGroup issues the same commands to every member.
// =============================================================================

#pragma once
#include "Nezumi/World/Entity/EntityID.hpp"
#include "Nezumi/World/Physics/Position2D.hpp"
#include <cstdint> // uint8_t

namespace Nezumi {

enum class UnitCommandType : std::uint8_t {
    Stop,       // stand still; engage enemies inside aggro_radius
    MoveTo,     // walk to `position`, ignoring enemies
    Attack,     // chase and attack `target` until it dies
    AttackMove, // walk to `position`, engaging enemies inside aggro_radius
};

struct UnitCommand {
    UnitCommandType type     = UnitCommandType::Stop;
    Position2D      position = {}; // MoveTo, AttackMove
    EntityHandle    target   = {}; // Attack

    static constexpr UnitCommand stop() {
        return {};
    }

    static constexpr UnitCommand move_to(Position2D pos) {
        return { UnitCommandType::MoveTo, pos, {} };
    }

    static constexpr UnitCommand attack(EntityHandle tgt) {
        return { UnitCommandType::Attack, {}, tgt };
    }

    static constexpr UnitCommand attack_move(Position2D pos){
        return { UnitCommandType::AttackMove, pos, {} };
    }
};

} // namespace Nezumi
