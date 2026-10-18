// =============================================================================
// World.cpp
// =============================================================================

#include "Nezumi/World/World.hpp"

#include "Nezumi/Core/Clock.hpp"
#include "Nezumi/World/Events/OnSurfaceCreated.hpp"
#include "Nezumi/World/Events/OnEntityCreated.hpp"
#include "Nezumi/World/Events/OnEntityRemoved.hpp"
#include "Nezumi/EngineContext.hpp"
#include <cstdint> // uint32_t

namespace Nezumi {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

World::World(Clock& clock, EventBus& event_bus, const WorldConfig& config)
    : clock_    (clock      )
    , event_bus_(event_bus  )
    , config_   (config     )
    , random_   (config.seed)
{}

World::~World() {
    // Entities are linked into chunk lists and sectors owned by surfaces.
    // Unlink everything first so teardown order cannot leave a list or node
    // pointing at freed memory, whatever IntrusiveList does on destruction.
    entities_.for_each([&](Entity& entity) {
        if (entity.chunk_id || entity.sector_span)
            get_surface(entity.surface_id)->remove_entity_(&entity);
    });
    entities_.clear();
    surfaces_.clear();
}

// -----------------------------------------------------------------------------
// Simulation
// -----------------------------------------------------------------------------

void World::tick() {
    assert(!in_tick_ && "World::tick is not re-entrant");
    in_tick_ = true;

    WorldContext ctx = make_context_();

    apply_commands_   ();       // 1. Commands
    update_activation_();       // 2. Activation
    tick_entities_    (ctx);    // 3. Entity tick
    // resolve_damage_   ();       // 4. Damage
    integrate_movement_(ctx.dt);// 5. Movement + spatial update
    update_activation_();       // 6. Activation
    sweep_sectors_    ();       // 7. Sector sweep

    tick_++;
    in_tick_ = false;
}

WorldContext World::make_context_() {
    return WorldContext{
        .world  = this,
        .random = &random_,
        .tick   = tick_,
        .dt     = static_cast<float>(clock_.get_fixed_dt()),
    };
}

// -----------------------------------------------------------------------------
// Surfaces
// -----------------------------------------------------------------------------

SurfaceID World::create_surface(const SurfaceConfig& config) {
    assert(surfaces_.count() < SurfaceID::max_value() && "SurfaceID space exhausted");

    const SurfaceID id{ static_cast<SurfaceID::value_t>(surfaces_.count()) };
    auto pair = surfaces_.create(/*.world = */*this, id, config);

    OnSurfaceCreated event;
    event.surface_id = id;
    event.tick      = tick_;
    publish_(event);

    pair.object->create_chunks(config.initial_chunks);
    return id;
}

Surface* World::get_surface(SurfaceID id) {
    // TODO: doublecheck this
    if (!id || id.value() >= surfaces_.count()) return nullptr;
    return surfaces_.get_by_index(static_cast<uint32_t>(id.value()));
}

const Surface* World::get_surface(SurfaceID id) const {
    // TODO: doublecheck this
    if (!id || id.value() >= surfaces_.count()) return nullptr;
    return surfaces_.get_by_index(static_cast<uint32_t>(id.value()));
}

// -----------------------------------------------------------------------------
// Entities
// -----------------------------------------------------------------------------

void World::remove_entity(EntityHandle handle) {
    Entity* entity = entities_.get(handle);
    if (!entity || entity->is_pending_remove()) return;

    entity->set_pending_remove();
    commands_.defer_entity_remove({ handle });
}

void World::set_entity_active(EntityHandle handle, bool active) {
    commands_.defer_entity_set_active({ handle, active });
}

// void World::damage_entity(EntityHandle target, uint16_t amount, EntityHandle source) {
//     if (amount == 0) return;
//     pending_damage_.push_back({ target, source, amount });
// }

// -----------------------------------------------------------------------------
// Tick Phase 1: Commands
// -----------------------------------------------------------------------------

void World::apply_commands_() {

    // Apply entity creates: insert into chunk and sector grids.
    commands_.apply_entity_creates([&](const auto& cmd) {
        EntityHandle handle = cmd.handle;
        Entity* entity = entities_.get(handle);
        if (!entity) return;

        // Destroyed before it ever joined the world: the destroy pass below
        // releases it without touching the grids.
        if (entity->is_pending_remove()) return;

        if (entity->is_active() && !entity->is_tickable())
            entity->clear_active();

        Surface* surface = get_surface(entity->surface_id);
        surface->insert_entity_(entity);

        entity->clear_pending_create();

        OnEntityCreated event;
        event.handle     = handle;
        event.type_id    = entity->type_id;
        event.surface_id = entity->surface_id;
        event.tick       = tick_;
        publish_(event);
    });

    // Apply entity activation changes: move between a chunk's active and inactive lists.
    commands_.apply_entity_set_actives([&](const auto& cmd) {
        Entity* entity = entities_.get(cmd.handle);
        if (!entity || !entity->is_alive())       return;
        if (entity->is_active() == cmd.active)    return;
        if (cmd.active && !entity->is_tickable()) return;

        Surface* surface = get_surface(entity->surface_id);
        surface->set_entity_active_(entity, cmd.active);
    });

    // Apply entity removes: remove from grids, notify, release the slot.
    commands_.apply_entity_removes([&](const auto& cmd) {
        EntityHandle handle = cmd.handle;
        Entity* entity = entities_.get(handle);
        if (!entity) return;

        get_surface(entity->surface_id)->remove_entity_(entity);

        OnEntityRemoved event;
        event.handle    = handle;
        event.type_id   = entity->type_id;
        event.surface_id = entity->surface_id;
        event.tick      = tick_;
        publish_(event);

        entities_.remove(handle);
    });
}

// -----------------------------------------------------------------------------
// Tick Phases 2 and 6: Activation
// -----------------------------------------------------------------------------

void World::update_activation_() {
    surfaces_.for_each([&](Surface& surface) {
        surface.update_chunk_activation_();
    });
}

// -----------------------------------------------------------------------------
// Tick Phase 3: Entity Tick
//
// Surfaces in SurfaceID order. A surface created during this phase is skipped
// until the next tick (count snapshot).
// -----------------------------------------------------------------------------

void World::tick_entities_(WorldContext& ctx) {
    surfaces_.for_each([&](Surface& surface) {
        surface.tick_entities_(ctx);
    });
}

// -----------------------------------------------------------------------------
// Tick Phase 4: Damage
// -----------------------------------------------------------------------------

// TODO

// void World::resolve_damage_() {
//     for (const DamageCommand_& cmd : std::exchange(pending_damage_, {})) {
//         Entity* target = entities_.get(cmd.target);
//         if (!target || !target->is_alive()) continue;
//         if (!target->is_damageable())       continue;
//         if (target->health == 0)            continue;

//         const uint16_t applied = std::min(cmd.amount, target->health);
//         target->health = static_cast<uint16_t>(target->health - applied);

//         OnEntityDamaged event;
//         event.target = cmd.target;
//         event.source = cmd.source;
//         event.amount = applied;
//         event.health = target->health;
//         event.tick   = tick_;
//         publish_(event);

//         // Flagged now, removed at the next command phase.
//         if (target->health == 0) destroy_entity(cmd.target);
//     }
// }

// -----------------------------------------------------------------------------
// Tick Phase 5: Movement
//
// Integrate every moving entity first, then update spatial membership, so
// no chunk list is modified while it is being walked.
// -----------------------------------------------------------------------------

void World::integrate_movement_(float dt) {
    surfaces_.for_each([&](Surface& surface) {
        moved_.clear();
        surface.integrate_movement_(dt, moved_);
        for (Entity* entity : moved_) surface.update_entity_(entity);
    });
    moved_.clear();
}

// -----------------------------------------------------------------------------
// Tick Phase 7: Sector Sweep
// -----------------------------------------------------------------------------

void World::sweep_sectors_() {
    const Tick interval = config_.sector_sweep_interval;
    if (interval == 0 || tick_ % interval != interval - 1) return;

    surfaces_.for_each([&](Surface& surface) {
        surface.sweep_empty_sectors_();
    });
}

} // namespace Nezumi
