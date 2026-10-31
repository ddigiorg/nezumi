// =============================================================================
// examples/04-unit-group-combat/main.cpp
//
// This example shows UnitGroups: Units marching and fighting in formation.
//
// - Red and blue each field a few groups of soldiers and archers.
//   Units spawn scattered, assemble into a block (archers in the back rows),
//   and attack-move across the field along their lane. Red has four groups
//   of 42 and blue three of 56, so the lanes don't line up.
// - A group holds formation while it marches. When its scan finds an enemy,
//   the formation breaks and every member fights on its own. Once no enemy
//   has been seen for a while, the survivors reform and march on.
// - Stragglers catch up at full speed while the group marches below its
//   slowest member's speed, and the march slows while many members are out
//   of place.
// - Groups that finish their route and stand idle attack the nearest enemy
//   group (UnitCommand::attack on a group handle). Groups that shrink below
//   MERGE_BELOW members merge into the nearest friendly group.
// - The overlay (F) shows each group's bounds colored by state, its slots,
//   its route, and a line from every straggler to its slot.
//
// Controls: right click sends the red groups attack-moving to the cursor
// (keeping their spacing); shift + right click adds a waypoint to their
// route instead; ctrl + right click is a plain move (formation never
// breaks, enemies are ignored: a retreat). F toggles the overlay, R restarts the battle. W/A/S/D or
// arrow keys pan, middle mouse drag pans, mouse wheel zooms toward the
// cursor. Press Escape (or close the window) to quit.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Engine.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnKeyUp.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Render/2D/Kinds/Circle.hpp"
#include "Nezumi/Render/2D/Kinds/Line.hpp"
#include "Nezumi/Render/2D/Kinds/Rectangle.hpp"
#include "Nezumi/World/Entities/Unit.hpp"
#include "Nezumi/World/Entities/UnitConfig.hpp"
#include "Nezumi/World/Entities/UnitGroup.hpp"
#include "Nezumi/World/Entities/UnitGroupConfig.hpp"
#include "Nezumi/World/Physics/Area2D.hpp"
#include "Nezumi/World/Physics/TileCoord.hpp"
#include <algorithm>   // find, max, min
#include <array>       // array
#include <cstdint>     // int32_t, uint16_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <optional>    // optional
#include <string>      // string, to_string
#include <utility>     // swap
#include <vector>      // vector

using namespace Nezumi;

// -----------------------------------------------------------------------------
// Input Actions
//
// Bound to keys in on_init. The engine names no actions: each game (here, each
// example) declares its own.
// -----------------------------------------------------------------------------

namespace Action {
    inline constexpr ActionID PanLeft  = 0;
    inline constexpr ActionID PanRight = 1;
    inline constexpr ActionID PanDown  = 2;
    inline constexpr ActionID PanUp    = 3;
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// Screen pixels per tile at zoom 1.0
static constexpr float TILE_PIXELS = 32.0f;

// Each side spawns one group per lane, SPAWN_X tiles from the middle, units
// scattered within SPAWN_HALF_SIZE tiles of the lane center. Same head count
// per side, split differently, so lanes don't line up and fights don't end
// in neat pairs: winners reform and go looking for the next group.
static constexpr size_t SIDE_COUNT      = 2;
static constexpr float  SPAWN_X         = 44.0f;
static constexpr float  SPAWN_HALF_SIZE = 5.0f;

struct SideSetup {
    std::array<float, 4> lanes_y;
    size_t               lane_count;
    size_t               soldiers; // per group
    size_t               archers;  // per group
};

static constexpr std::array<SideSetup, SIDE_COUNT> SETUPS = {{
    { { -27.0f, -9.0f, 9.0f, 27.0f }, 4, 30, 12 }, // red:  4 x 42
    { { -18.0f,  0.0f, 18.0f, 0.0f }, 3, 40, 16 }, // blue: 3 x 56
}};

// Ground pre-created in [-MAP_HALF, MAP_HALF)
static constexpr glm::vec2 MAP_HALF = { 64.0f, 40.0f };

// Idle groups (route done, nothing in sight) attack the nearest enemy group
static constexpr uint16_t IDLE_RETARGET_TICKS = 120; // 2 s at 60 UPS

// Groups smaller than this merge into the nearest friendly group
static constexpr uint16_t MERGE_BELOW = 12;

// -----------------------------------------------------------------------------
// Configs
//
// Shared stats. Static storage, so they outlive every unit and group that
// points at them.
// -----------------------------------------------------------------------------

static constexpr UnitConfig SOLDIER = {
    .name            = "soldier",
    .subtype         = EntitySubtypeID{ 0 },
    .max_health      = 100,
    .move_speed      = 2.5f,  // tiles per second
    .attack_damage   = 10,
    .attack_interval = 60,    // ticks (1 hit per second)
    .attack_range    = 0.75f, // tiles
    .aggro_radius    = 6.0f,  // tiles
};

static constexpr UnitConfig ARCHER = {
    .name            = "archer",
    .subtype         = EntitySubtypeID{ 1 },
    .max_health      = 60,
    .move_speed      = 2.2f,
    .attack_damage   = 6,
    .attack_interval = 45,
    .attack_range    = 4.0f,
    .aggro_radius    = 7.0f,
};

// Columns from aspect 2:1 (42 members: 9 x 5, 56: 11 x 6), archers in the
// back rows
static constexpr UnitGroupConfig BLOCK = {
    .name    = "block",
    .spacing = 0.9f,
    .aspect  = 2.0f,
};

// -----------------------------------------------------------------------------
// Atlas
//
// Pixel boxes in assets/graphics/atlas.png { x0, y0, x1, y1 }, y down.
// -----------------------------------------------------------------------------

namespace AtlasPixels {
    // 64x64 px ground tiles, drawn 1x1 tile
    constexpr glm::vec4 GrassDark  = {  64.0f, 256.0f, 128.0f, 320.0f };
    constexpr glm::vec4 GrassLight = {  64.0f, 320.0f, 128.0f, 384.0f };

    // 64x128 px unit frames, drawn 0.5x1 tile. One row of 5 headings per
    // color: down, down-right, right, up-right, up (left ones are mirrored).
    constexpr glm::vec2 UnitSize = { 64.0f, 128.0f };
}

// One style per side: FactionIndex indexes these arrays directly
struct SideStyle {
    const char* name;
    glm::vec2   atlas_origin; // first unit frame in the atlas
    glm::vec4   color;        // overlay tint
    float       direction;    // +1 marches right, -1 left
};

static constexpr std::array<SideStyle, SIDE_COUNT> SIDES = {{
    { "red",  { 0.0f,   0.0f }, { 1.0f, 0.35f, 0.3f, 1.0f },  1.0f },
    { "blue", { 0.0f, 128.0f }, { 0.4f, 0.6f,  1.0f, 1.0f }, -1.0f },
}};

static constexpr FactionIndex RED = 0;

constexpr glm::vec4 inset_half_texel(glm::vec4 pixel_box) {
    return pixel_box + glm::vec4{ 0.5f, 0.5f, -0.5f, -0.5f };
}

// Unit sprite size in tiles, anchored on its feet (the unit's position)
static constexpr glm::vec4 UNIT_SPRITE_BOUNDS = { -0.25f, -0.25f, 0.25f, 0.75f };

// Healthbar above the sprite, drawn only once a unit is hurt
static constexpr glm::vec4 HEALTHBAR_BOUNDS = { -0.3f, 0.82f, 0.3f, 0.97f };
static constexpr float     HEALTHBAR_BORDER = 0.04f;

// Overlay colors by UnitGroupState
static constexpr std::array<glm::vec4, 3> STATE_COLORS = {{
    { 1.0f, 1.0f, 1.0f, 0.8f }, // Formed
    { 1.0f, 0.3f, 0.1f, 0.9f }, // Engaged
    { 1.0f, 0.9f, 0.2f, 0.9f }, // Reforming
}};

static constexpr std::array<const char*, 3> STATE_NAMES = { "formed", "engaged", "reforming" };

static constexpr glm::vec4 STRAGGLER_COLOR = { 1.0f, 0.9f, 0.2f, 0.9f };

// -----------------------------------------------------------------------------
// Draw Params
// -----------------------------------------------------------------------------

constexpr Render::DrawParams DRAW_PARAMS_GROUND = {
    .view  = Render::View::World,
    .layer = 0,
};

// Under the units, so slots and routes read as markings on the ground
constexpr Render::DrawParams DRAW_PARAMS_OVERLAY_GROUND = {
    .view  = Render::View::World,
    .layer = 1,
};

constexpr Render::DrawParams DRAW_PARAMS_UNITS = {
    .view  = Render::View::World,
    .layer = 2,
};

constexpr Render::DrawParams DRAW_PARAMS_HEALTHBARS = {
    .view  = Render::View::World,
    .layer = 3,
};

constexpr Render::DrawParams DRAW_PARAMS_OVERLAY = {
    .view  = Render::View::World,
    .layer = 4,
};

constexpr Render::DrawParams DRAW_PARAMS_UI = {
    .view  = Render::View::Screen,
    .layer = 0,
};

// -----------------------------------------------------------------------------
// UnitGroupCombatExample
// -----------------------------------------------------------------------------

class UnitGroupCombatExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::World&    world    = engine.world();
        Render::Renderer2D* renderer = engine.renderer2d();

        // Setup cameras
        world_camera_  = renderer->get_view_camera(Render::View::World );
        screen_camera_ = renderer->get_view_camera(Render::View::Screen);
        world_camera_->set_pixels_per_unit(TILE_PIXELS);

        // Keys that pan the camera
        InputMap& input = engine.input();
        input.bind(Action::PanLeft,  { ScanCode::A, ScanCode::Left  });
        input.bind(Action::PanRight, { ScanCode::D, ScanCode::Right });
        input.bind(Action::PanDown,  { ScanCode::S, ScanCode::Down  });
        input.bind(Action::PanUp,    { ScanCode::W, ScanCode::Up    });

        camera_controller_ = std::make_unique<CameraController>(
            /*.events   = */engine.event_bus(),
            /*.input    = */input,
            /*.renderer = */*renderer,
            /*.camera   = */renderer->get_view(Render::View::World),
            CameraControllerConfig{
                .zoom_init = 0.3f, // the whole field fits at 1280 px
                .zoom_min  = 0.125f,
                .zoom_max  = 4.0f,
                .pan_left  = Action::PanLeft,
                .pan_right = Action::PanRight,
                .pan_down  = Action::PanDown,
                .pan_up    = Action::PanUp,
            }
        );

        // Input. Handlers only record what happened; on_tick applies it
        // between world ticks, which is where commands belong.
        listeners_.push_back(engine.event_bus().subscribe<OnMouseMotion>(
            [this](const OnMouseMotion& e) { cursor_ = { e.pos_x, e.pos_y }; }
        ));
        listeners_.push_back(engine.event_bus().subscribe<OnMouseButtonDown>(
            [this](const OnMouseButtonDown& e) {
                if (e.button != MouseButton::Right) return;
                const glm::vec2 p = world_camera_->screen_to_world(cursor_);
                pending_order_ = PendingOrder{ Position2D{ p.x, p.y }, shift_held_, ctrl_held_ };
            }
        ));
        listeners_.push_back(engine.event_bus().subscribe<OnKeyDown>(
            [this](const OnKeyDown& e) {
                if (e.scancode == ScanCode::LShift || e.scancode == ScanCode::RShift) shift_held_ = true;
                if (e.scancode == ScanCode::LCtrl  || e.scancode == ScanCode::RCtrl ) ctrl_held_  = true;
                if (e.repeat) return;
                if (e.scancode == ScanCode::R) pending_reset_ = true;
                if (e.scancode == ScanCode::F) show_overlay_ = !show_overlay_;
            }
        ));
        listeners_.push_back(engine.event_bus().subscribe<OnKeyUp>(
            [this](const OnKeyUp& e) {
                if (e.scancode == ScanCode::LShift || e.scancode == ScanCode::RShift) shift_held_ = false;
                if (e.scancode == ScanCode::LCtrl  || e.scancode == ScanCode::RCtrl ) ctrl_held_  = false;
            }
        ));

        // Setup sprite atlas and font
        atlas_ = renderer->create_atlas(
            Render::AtlasConfig{
                .name     = "group-atlas",
                .filepath = "assets/graphics/atlas.png",
            }
        ).object;

        tex_grass_dark_  = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassDark ));
        tex_grass_light_ = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassLight));

        font_ = renderer->create_font(
            Render::FontConfig{
                .name     = "group-font",
                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf"
            }
        ).object;

        surface_id_ = world.create_surface(
            SurfaceConfig{
                .name           = "battlefield",
                .initial_chunks = ChunkSpan::span_from_box(
                    Area2D(-MAP_HALF.x, -MAP_HALF.y, MAP_HALF.x, MAP_HALF.y)),
            }
        );

        spawn_sides_(engine);

        engine.logger().info("Unit Group Combat Example");
        engine.logger().info("Right click: red attack-move. Shift: add waypoint. Ctrl: move (no fighting).");
        engine.logger().info("F: overlay. R: restart. Escape or close the window to exit.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // Runs before World::tick, so commands issued here take effect this tick.
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {
        Nezumi::World& world = engine.world();

        if (pending_reset_) {
            pending_reset_ = false;
            clear_sides_(world);
            spawn_sides_(engine);
        }

        if (pending_order_) {
            order_red_(world, *pending_order_);
            pending_order_.reset();
        }

        prune_groups_(world);
        merge_small_groups_(world);
        retarget_idle_groups_(world);
    }

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {
        Render::Renderer2D* r = engine.renderer2d();

        camera_controller_->update(engine.clock().get_real_dt());

        const Nezumi::World&   world   = engine.world();
        const Nezumi::Surface* surface = world.get_surface(surface_id_);
        if (!surface) return;

        // Only visit chunks the camera can see
        const glm::vec4 view = world_camera_->world_bounds();
        const Area2D    view_area(view.x, view.y, view.z, view.w);
        const ChunkSpan chunk_span = ChunkSpan::span_from_box(view_area);

        surface->each_chunk_in(chunk_span, [&](const Chunk& chunk) {
            draw_ground_(r, chunk, view_area);
            draw_units_ (r, chunk);
        });

        if (show_overlay_) draw_overlay_(r, world, view_area);

        draw_hud_(r, engine);
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
        for (EventListenerID id : listeners_) engine.event_bus().unsubscribe(id);
        camera_controller_.reset();

        engine.logger().info("Ran " + std::to_string(engine.get_stats().tick_count) + " ticks.");
        engine.logger().info("Goodbye!");
    }

private:

    struct PendingOrder {
        Position2D position;
        bool       add_waypoint; // shift
        bool       move_only;    // ctrl
    };

    // -------------------------------------------------------------------------
    // Spawn
    //
    // One group per lane per side. Units spawn scattered around the lane and
    // join the group right away; the group's first layout gathers them into
    // a block facing its destination before it marches.
    // -------------------------------------------------------------------------

    void spawn_sides_(Nezumi::Engine& engine) {
        Nezumi::World& world = engine.world();

        for (FactionIndex f = 0; f < SIDE_COUNT; f++) {
            const float      dir   = SIDES[f].direction;
            const SideSetup& setup = SETUPS[f];

            for (size_t lane = 0; lane < setup.lane_count; lane++) {
                const float lane_y = setup.lanes_y[lane];
                const Position2D center      = { -dir * SPAWN_X, lane_y };
                const Position2D destination = {  dir * SPAWN_X, lane_y };

                const EntityHandle group_handle = world.create_entity<UnitGroup>(
                    /*.surface  = */surface_id_,
                    /*.position = */center,
                    /*.config   = */BLOCK,
                    /*.factions = */faction_bit(f)
                );
                UnitGroup* group = world.get_entity_as<UnitGroup>(group_handle);

                for (size_t i = 0; i < setup.soldiers + setup.archers; i++) {
                    const UnitConfig& config = (i < setup.soldiers) ? SOLDIER : ARCHER;

                    const Position2D position = {
                        center.x + engine.random().get<float>(-SPAWN_HALF_SIZE, SPAWN_HALF_SIZE),
                        center.y + engine.random().get<float>(-SPAWN_HALF_SIZE, SPAWN_HALF_SIZE),
                    };

                    const EntityHandle handle = world.create_entity<Unit>(
                        /*.surface  = */surface_id_,
                        /*.position = */position,
                        /*.config   = */config,
                        /*.factions = */faction_bit(f)
                    );

                    // Unit and group join the world next tick, together, but
                    // both exist now, so membership can be set up already
                    group->add_member(*world.get_entity_as<Unit>(handle));
                }

                group->attack_move(destination);
                groups_.push_back(group_handle);
            }
        }
    }

    void clear_sides_(Nezumi::World& world) {
        std::vector<EntityHandle> doomed;
        world.each_entity([&](const Nezumi::Entity& entity) {
            if (entity.is_type<Unit>() || entity.is_type<UnitGroup>()) doomed.push_back(entity.handle);
        });
        for (EntityHandle handle : doomed) world.remove_entity(handle);
        groups_.clear();
    }

    // -------------------------------------------------------------------------
    // Orders
    // -------------------------------------------------------------------------

    // Red groups attack-move to (move to, or add a waypoint at)
    // `order.position`, each keeping its offset from the red groups' center
    // so they don't pile up.
    void order_red_(Nezumi::World& world, const PendingOrder& order) {
        Position2D sum   = {};
        float      count = 0.0f;
        for_each_group_(world, [&](UnitGroup& group) {
            if (group.faction_index() != RED) return;
            sum   += group.position;
            count += 1.0f;
        });
        if (count == 0.0f) return;

        const Position2D center = sum / count;

        for_each_group_(world, [&](UnitGroup& group) {
            if (group.faction_index() != RED) return;
            const Position2D target = order.position + (group.position - center);
            if      (order.add_waypoint) group.add_waypoint(target);
            else if (order.move_only)    group.move_to(target);
            else                         group.attack_move(target);
        });
    }

    // A group that shrank below MERGE_BELOW joins its nearest friend, unless
    // it is in the middle of a fight.
    // member_count() updates on the group's tick, so groups merged away
    // this call are tracked here.
    void merge_small_groups_(Nezumi::World& world) {
        std::vector<const UnitGroup*> merged;
        const auto was_merged = [&](const UnitGroup& g) {
            return std::find(merged.begin(), merged.end(), &g) != merged.end();
        };

        for_each_group_(world, [&](UnitGroup& small) {
            if (small.member_count() >= MERGE_BELOW)      return;
            if (small.state() == UnitGroupState::Engaged) return;

            UnitGroup* nearest = nullptr;
            float      best    = 0.0f;
            for_each_group_(world, [&](UnitGroup& other) {
                if (&other == &small || !small.is_friend(other) || was_merged(other)) return;
                const float d2 = small.distance2(other);
                if (!nearest || d2 < best) { nearest = &other; best = d2; }
            });

            if (!nearest) return;
            nearest->merge_from(world, small);
            merged.push_back(&small);
        });
    }

    // Groups that finished their route and found nobody attack the nearest
    // enemy group, wherever it is.
    void retarget_idle_groups_(Nezumi::World& world) {
        for_each_group_(world, [&](UnitGroup& group) {
            if (group.state() != UnitGroupState::Formed)          return;
            if (group.command().type != UnitCommandType::Stop)    return;
            if (group.state_ticks() < IDLE_RETARGET_TICKS)        return;

            const UnitGroup* nearest = nullptr;
            float            best    = 0.0f;
            for_each_group_(world, [&](UnitGroup& other) {
                if (!group.is_enemy(other)) return;
                const float d2 = group.distance2(other);
                if (!nearest || d2 < best) { nearest = &other; best = d2; }
            });

            if (nearest) group.attack(nearest->handle);
        });
    }

    // Drop groups that removed themselves (no members left)
    void prune_groups_(Nezumi::World& world) {
        std::erase_if(groups_, [&](EntityHandle handle) {
            const UnitGroup* group = world.get_entity_as<UnitGroup>(handle);
            return !group || group->is_pending_remove();
        });
    }

    // Every group we spawned that still exists. Safe to nest: it never
    // changes groups_.
    template <typename Fn>
    void for_each_group_(Nezumi::World& world, Fn&& fn) {
        for (EntityHandle handle : groups_) {
            UnitGroup* group = world.get_entity_as<UnitGroup>(handle);
            if (group && !group->is_pending_remove()) fn(*group);
        }
    }

    // -------------------------------------------------------------------------
    // Draw Ground
    // -------------------------------------------------------------------------

    void draw_ground_(Render::Renderer2D* r, const Chunk& chunk, const Area2D& view) {
        const Area2D& bounds = chunk.bounds();

        const TileCoord min = TileCoord::from_position({
            std::max(bounds.min.x, view.min.x),
            std::max(bounds.min.y, view.min.y),
        });
        const TileCoord max = TileCoord::from_position({
            std::min(bounds.max.x, view.max.x) - 0.001f,
            std::min(bounds.max.y, view.max.y) - 0.001f,
        });

        Render::DrawParams params = DRAW_PARAMS_GROUND;
        params.material = atlas_->material();

        TileCoord tile;
        for (tile.y = min.y; tile.y <= max.y; tile.y++) {
            for (tile.x = min.x; tile.x <= max.x; tile.x++) {
                r->draw_sprite(
                    Render::Sprite{
                        .pos_box = tile.bounds().to_vec4(),
                        .tex_box = ((tile.x + tile.y) & 1) ? tex_grass_light_ : tex_grass_dark_,
                    },
                    params
                );
            }
        }
    }

    // -------------------------------------------------------------------------
    // Draw Units
    //
    // Sprite faces Unit::facing(), sorted by y. Healthbars only on hurt units,
    // so a marching block reads as a block.
    // -------------------------------------------------------------------------

    void draw_units_(Render::Renderer2D* r, const Chunk& chunk) {
        Render::DrawParams params = DRAW_PARAMS_UNITS;
        params.material = atlas_->material();

        chunk.visit_active_entities([&](const Nezumi::Entity& entity) {
            if (!entity.is_alive())      return true;
            if (!entity.is_type<Unit>()) return true;
            const Unit& unit = *entity.as_type<Unit>();

            const glm::vec2 pos = unit.position.to_vec2();

            // Heading column: 0 down, 1 down-right, 2 right, 3 up-right, 4 up.
            // 0.383 = sin(22.5 deg) splits the circle into 8 equal sectors.
            const glm::vec2 facing = unit.facing().to_vec2();
            const int dx = (facing.x > 0.383f) - (facing.x < -0.383f);
            const int dy = (facing.y > 0.383f) - (facing.y < -0.383f);
            const int column = (dx == 0) ? (dy > 0 ? 4 : 0) : (dy + 2);

            const glm::vec2 origin = SIDES[unit.faction_index()].atlas_origin
                                   + glm::vec2{ column * AtlasPixels::UnitSize.x, 0.0f };
            glm::vec4 tex_box = atlas_->tex_box({ origin, origin + AtlasPixels::UnitSize });
            if (dx < 0) std::swap(tex_box.x, tex_box.z); // mirror

            params.order = -static_cast<int32_t>(pos.y * 16.0f);

            r->draw_sprite(
                Render::Sprite{
                    .pos_box = glm::vec4{ pos, pos } + UNIT_SPRITE_BOUNDS,
                    .tex_box = tex_box,
                },
                params
            );

            if (!unit.is_full_health()) draw_healthbar_(r, pos, unit.health_ratio());
            return true;
        });
    }

    void draw_healthbar_(Render::Renderer2D* r, glm::vec2 pos, float ratio) {
        const glm::vec4 back = glm::vec4{ pos, pos } + HEALTHBAR_BOUNDS;

        glm::vec4 fill = back + glm::vec4{ HEALTHBAR_BORDER, HEALTHBAR_BORDER,
                                          -HEALTHBAR_BORDER, -HEALTHBAR_BORDER };
        fill.z = fill.x + (fill.z - fill.x) * ratio;

        r->draw_rectangle(Render::Rectangle{ .pos_box = back, .color = { 0.0f, 0.0f, 0.0f, 1.0f } },
                          DRAW_PARAMS_HEALTHBARS);
        r->draw_rectangle(Render::Rectangle{ .pos_box = fill, .color = { 1.0f, 1.0f, 1.0f, 1.0f } },
                          DRAW_PARAMS_HEALTHBARS);
    }

    // -------------------------------------------------------------------------
    // Draw Overlay
    //
    // Per group: bounds (by state), slots (Formed and Reforming), the route,
    // and a line from each straggler to its slot. Reads the plan for the
    // coming tick, which is what members will steer toward next.
    // -------------------------------------------------------------------------

    void draw_overlay_(Render::Renderer2D* r, const Nezumi::World& world, const Area2D& view) {
        const Tick tick = world.get_tick();

        for (EntityHandle handle : groups_) {
            const UnitGroup* group = world.get_entity_as<UnitGroup>(handle);
            if (!group || !group->is_alive()) continue;

            const Area2D box = group->collision_box();
            if (!box.overlaps(Area2D(view).grow(8.0f))) continue;

            const glm::vec4 side_color  = SIDES[group->faction_index()].color;
            const glm::vec4 state_color = STATE_COLORS[static_cast<size_t>(group->state())];
            const bool      engaged     = group->state() == UnitGroupState::Engaged;

            // Bounds, padded so units on the edge sit inside
            const Area2D frame = Area2D(box).grow(0.6f);
            draw_frame_(r, frame, state_color, 0.08f);

            // Route: anchor -> waypoints
            glm::vec2 from = group->position.to_vec2();
            for (Position2D waypoint : group->waypoints()) {
                const glm::vec2 to = waypoint.to_vec2();
                r->draw_line(Render::Line{ .position_a = from, .position_b = to,
                                           .color = side_color * glm::vec4(1, 1, 1, 0.6f), .width = 0.1f },
                             DRAW_PARAMS_OVERLAY_GROUND);
                r->draw_circle(Render::Circle{ .position = to, .color = side_color,
                                               .radius = 0.35f, .width = 0.1f },
                               DRAW_PARAMS_OVERLAY_GROUND, /*.filled = */false);
                from = to;
            }

            // Anchor and facing
            const glm::vec2 anchor = group->position.to_vec2();
            const glm::vec2 facing = group->plan(tick).facing.to_vec2();
            r->draw_line(Render::Line{ .position_a = anchor, .position_b = anchor + facing * 1.5f,
                                       .color = state_color, .width = 0.12f },
                         DRAW_PARAMS_OVERLAY);
            r->draw_circle(Render::Circle{ .position = anchor, .color = state_color, .radius = 0.2f, .width = 1.0f },
                           DRAW_PARAMS_OVERLAY);

            if (engaged) {
                const glm::vec2 enemy = group->plan(tick).engage_position.to_vec2();
                r->draw_line(Render::Line{ .position_a = anchor, .position_b = enemy,
                                           .color = state_color * glm::vec4(1, 1, 1, 0.5f), .width = 0.06f },
                             DRAW_PARAMS_OVERLAY);
                continue; // nobody holds a slot while engaged
            }

            // Slots, and stragglers' lines to theirs
            const std::vector<EntityHandle>& members = group->members();
            for (size_t i = 0; i < members.size(); i++) {
                const Unit* unit = world.get_entity_as<Unit>(members[i]);
                if (!unit || unit->group() != handle) continue;

                const glm::vec2 slot = group->slot_position(tick, static_cast<uint16_t>(i)).to_vec2();
                r->draw_circle(Render::Circle{ .position = slot, .color = side_color * glm::vec4(1, 1, 1, 0.5f),
                                               .radius = 0.22f, .width = 0.05f },
                               DRAW_PARAMS_OVERLAY_GROUND, /*.filled = */false);

                const glm::vec2 pos = unit->position.to_vec2();
                const glm::vec2 lag = slot - pos;
                const float     straggle = group->config().straggler_distance;
                if (lag.x * lag.x + lag.y * lag.y > straggle * straggle) {
                    r->draw_line(Render::Line{ .position_a = pos, .position_b = slot,
                                               .color = STRAGGLER_COLOR, .width = 0.06f },
                                 DRAW_PARAMS_OVERLAY);
                }
            }
        }
    }

    void draw_frame_(Render::Renderer2D* r, const Area2D& box, glm::vec4 color, float width) {
        const glm::vec2 a = { box.min.x, box.min.y };
        const glm::vec2 b = { box.max.x, box.min.y };
        const glm::vec2 c = { box.max.x, box.max.y };
        const glm::vec2 d = { box.min.x, box.max.y };
        for (auto [p, q] : { std::pair{ a, b }, std::pair{ b, c }, std::pair{ c, d }, std::pair{ d, a } }) {
            r->draw_line(Render::Line{ .position_a = p, .position_b = q, .color = color, .width = width },
                         DRAW_PARAMS_OVERLAY);
        }
    }

    // -------------------------------------------------------------------------
    // Draw HUD
    //
    // Units and groups per side, then one line per group.
    // -------------------------------------------------------------------------

    void draw_hud_(Render::Renderer2D* r, Nezumi::Engine& engine) {
        const Nezumi::World& world = engine.world();

        std::array<size_t, SIDE_COUNT> units  = {};
        std::array<size_t, SIDE_COUNT> groups = {};

        world.each_entity([&](const Nezumi::Entity& entity) {
            if (!entity.is_alive()) return;
            if (entity.is_type<Unit>())      units [entity.faction_index()]++;
            if (entity.is_type<UnitGroup>()) groups[entity.faction_index()]++;
        });

        const Nezumi::Engine::LoopStats& stats = engine.get_stats();

        std::string text;
        char line[128];

        for (size_t f = 0; f < SIDE_COUNT; f++) {
            std::snprintf(line, sizeof line, "%-5s %4zu units  %zu groups\n", SIDES[f].name, units[f], groups[f]);
            text += line;
        }
        text += "\n";

        for (EntityHandle handle : groups_) {
            const UnitGroup* group = world.get_entity_as<UnitGroup>(handle);
            if (!group || !group->is_alive()) continue;
            std::snprintf(line, sizeof line, "%-5s #%-3u %-9s %3u  stragglers %2u  lag %.2f\n",
                SIDES[group->faction_index()].name, static_cast<unsigned>(handle.index()),
                STATE_NAMES[static_cast<size_t>(group->state())],
                static_cast<unsigned>(group->member_count()),
                static_cast<unsigned>(group->straggler_count()), group->lag());
            text += line;
        }

        std::snprintf(line, sizeof line,
            "\ntick %.2f ms  draw %.2f ms\n"
            "right click: red attack-move  shift: add waypoint  ctrl: move\n"
            "F: overlay  R: restart",
            stats.tick_ms, stats.draw_ms);
        text += line;

        r->draw_text(
            text,
            { 8.0f, screen_camera_->get_size().y - 8.0f },
            *font_,
            Render::TextStyle {
                .size          = 16.0f,
                .color         = { 1.0f, 1.0f, 1.0f, 1.0f },
                .outline_color = { 0.0f, 0.0f, 0.0f, 1.0f },
                .outline_width = 3.0f,
                .align         = Render::TextAlign::Left,
                .valign        = Render::TextVAlign::Top,
            },
            DRAW_PARAMS_UI
        );
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Render::Camera2D* world_camera_  = nullptr;
    Render::Camera2D* screen_camera_ = nullptr;
    Render::Atlas*  atlas_         = nullptr;
    Render::Font*   font_          = nullptr;

    glm::vec4 tex_grass_dark_  = {};
    glm::vec4 tex_grass_light_ = {};

    std::unique_ptr<CameraController> camera_controller_ = nullptr;
    std::vector<EventListenerID>      listeners_         = {};
    glm::vec2                         cursor_            = { 0.0f, 0.0f }; // window pixels, y down
    bool                              shift_held_        = false;
    bool                              ctrl_held_         = false;
    bool                              show_overlay_      = true;

    // Input waiting for the next on_tick
    std::optional<PendingOrder> pending_order_ = {};
    bool                        pending_reset_ = false;

    SurfaceID                 surface_id_ = {};
    std::vector<EntityHandle> groups_     = {};
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title  = "Unit Group Combat Example";
        config.window.width  = 1280;
        config.window.height = 720;
        config.escape_exits = true;

        Nezumi::Engine engine(config);

        UnitGroupCombatExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
