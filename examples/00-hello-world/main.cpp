// =============================================================================
// examples/00-hello-world/main.cpp
//
// This example shows the smallest complete program you can build with Nezumi.
// These three things are required:
//
// 1. A class that implements IApplication. The engine calls into it at four
//    points in its lifetime: init, tick, draw and exit.
//
// 2. An EngineConfig describing how the engine should be set up. Every field
//    has a sensible default, so you only override what you care about.
//
// 3. An Engine that owns every subsystem and runs the main loop until the
//    application is closed.  Example subsystems include:
//
//    engine.logger()    - write messages to the console and/or a log file
//    engine.clock()     - timing information for the main loop
//    engine.random()    - seeded random number generation
//    engine.event_bus() - subscribe to and dispatch events
//    engine.renderer2d() - 2D drawing (nullptr when graphics or
//                          EngineConfig::enable_renderer2d are disabled)
//    engine.window()     - the OS window (nullptr when graphics are disabled)
//
// Run it, watch the log, then press Escape (or close the window) to quit.
// =============================================================================

#include "Nezumi/Engine.hpp"
#include <cstdlib>   // EXIT_FAILURE, EXIT_SUCCESS
#include <exception> // exception
#include <iostream>  // cerr
#include <string>    // string, to_string

class HelloWorldExample final : public Nezumi::IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    //
    // Called once, after the engine and all of its subsystems are ready but
    // before the main loop starts. Load assets and set up game state here.
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::Logger& logger = engine.logger();
        logger.info("Hello, World!");
        logger.info("Press Escape or close the window to exit.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // Called at a fixed rate (EngineConfig::clock.fixed_timestep, 60 times per
    // second by default), no matter how fast or slow frames are drawn. Put
    // game logic here: movement, physics, AI, input handling.
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {}

    // -------------------------------------------------------------------------
    // OnDraw
    //
    // Called once per frame, after any ticks for that frame. The engine has
    // already started the frame (clearing the screen to
    // EngineConfig::render.clear_color) and presents it after this returns,
    // so only issue draw calls here and keep game logic in on_tick.
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {}

    // -------------------------------------------------------------------------
    // OnExit
    //
    // Called once, after the main loop ends and before the engine is destroyed.
    // Save progress and release game resources here.
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
        Nezumi::Logger& logger = engine.logger();
        Nezumi::Engine::LoopStats stats = engine.get_stats();
        logger.info("Ran " + std::to_string(stats.tick_count) + " ticks.");
        logger.info("Ran " + std::to_string(stats.draw_count) + " draws.");
        logger.info("Goodbye!");
    }
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        // Start from the defaults and override only what this example needs.
        // See `src/EngineConfig.hpp` for every available option.
        Nezumi::EngineConfig config = {};
        config.window.title = "Hello, World!";
        config.escape_exits = true;

        // Constructing the Engine creates every subsystem. It throws if any of
        // them fail (for example, no compatible graphics driver).
        // See `src/Engine.hpp` for more information.
        Nezumi::Engine engine(config);

        // run() blocks until the game exits, then returns a status code that
        // we pass straight back to the operating system.
        HelloWorldExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
