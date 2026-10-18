// =============================================================================
// Unit.hpp
// =============================================================================

#pragma once
#include "Nezumi/Core/Random.hpp"
#include "Nezumi/World/Entity/Entity.hpp"
#include "Nezumi/World/WorldContext.hpp"
// #include <iostream>

using namespace Nezumi;

class Unit : public Nezumi::Entity {
public:
    static constexpr EntityTypeID TYPE_ID{ 1 };

    bool thing_;

    Unit(bool thing) {
        this->set_active();
        this->set_tickable();
        thing_ = thing;
        collision_bounds = { -0.5f, -0.5f, 0.5f, 0.5f };
    };

    void tick(WorldContext& ctx) override {
        // std::cout << "unit ticked" << std::endl;

        this->position.x += 0.1f * ctx.random->get<int>(-1, 1);
        this->position.y += 0.1f * ctx.random->get<int>(-1, 1);
    }
};
