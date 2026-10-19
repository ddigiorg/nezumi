// =============================================================================
// examples/02-world-2d/main.cpp
// =============================================================================

#include "Nezumi/Engine.hpp"
#include "Unit.hpp"
#include <cstdlib>   // EXIT_FAILURE, EXIT_SUCCESS
#include <exception> // exception
#include <iostream>  // cerr
#include <string>    // string, to_string

using namespace Nezumi; // TODO: should examples be more explicit?

// -----------------------------------------------------------------------------
// WorldExample
// -----------------------------------------------------------------------------

class WorldExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::Logger& logger = engine.logger();
        World&  world  = engine.world();
        Render::Renderer* renderer = engine.renderer();

        logger.info("Hello, World!");
        logger.info("Press Escape or close the window to exit.");

        // TODO: make size and position tile-space units
        // TODO: add set_position()
        // Render::Camera* player_camera = renderer->get_player_camera();
        // player_camera->set_zoom(16.0f);


        SurfaceID surface_id = world.create_surface(
            SurfaceConfig{
                .name           = "surface-0",
                .initial_chunks = {},
            }
        );

        // EntityHandle e1 = world.create_entity<Unit>(
        //     /*.surface  = */surface_id,
        //     /*.position = */{ 16.0f, 16.0f },
        //     /*.args     = */true
        // );

        for(size_t i = 0; i < 20000; i++) {
            EntityHandle e1 = world.create_entity<Unit>(
                /*.surface  = */surface_id,
                /*.position = */{
                    // TODO: random positions should be from world not engine
                    /*.x = */engine.random().get<float>(-16.0f, 16.0f),
                    /*.y = */engine.random().get<float>(-16.0f, 16.0f),
                },
                /*.args     = */true
            );
        }
    }

    // -------------------------------------------------------------------------
    // OnTick
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {}

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {
        const World& world = engine.world();
        Render::Renderer* renderer = engine.renderer();

        world.each_surface([&](const Nezumi::Surface& surface){
            surface.each_chunk([&](const Chunk& chunk){

                // Draw chunk
                renderer->draw_rectangle(
                    Render::Rectangle{
                        .pos_box = chunk.bounds().to_vec4(),
                        .color   = { 0.0f, 1.0f, 0.0f, 1.0f },
                    },
                    Render::DrawParams{
                        .view  = 0,
                        .layer = 0,
                    }
                );

                // Draw chunk entities
                chunk.visit_active_entities([&](const Nezumi::Entity& entity) {
                    if (entity.is_pending_remove()) return true;
                    renderer->draw_rectangle(
                        Render::Rectangle{
                            .pos_box = entity.collision_box().to_vec4(),
                            .color   = { 1.0f, 0.0f, 0.0f, 1.0f },
                        },
                        Render::DrawParams{
                            .view  = 0,
                            .layer = 1,
                        }
                    );
                    return true;
                });
            });
        });

        Nezumi::Engine::LoopStats stats = engine.get_stats();
        std::cout << stats.tick_ms << std::endl;
        std::cout << stats.draw_ms << std::endl;
        std::cout << std::endl;
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
        Nezumi::Logger& log = engine.logger();
        Nezumi::Engine::LoopStats stats = engine.get_stats();
        log.info("Ran " + std::to_string(stats.tick_count) + " ticks.");
        log.info("Ran " + std::to_string(stats.draw_count) + " draws.");
        log.info("Goodbye!");

        // std::cout << engine.world().surface_count() << std::endl;
        // std::cout << engine.world().entity_count()  << std::endl;
    }
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title = "World2D Example";
        config.escape_exits = true;

        Nezumi::Engine engine(config);

        WorldExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
