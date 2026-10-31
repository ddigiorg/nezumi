// =============================================================================
// Engine.cpp
// =============================================================================

#include "Nezumi/Core/Events/OnGameDrawn.hpp"
#include "Nezumi/Core/Events/OnGameExited.hpp"
#include "Nezumi/Core/Events/OnGameInitialized.hpp"
// #include "Nezumi/Core/Events/OnGamePaused.hpp"
#include "Nezumi/Core/Events/OnGameTicked.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Engine.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include <memory> // make_unique

namespace Nezumi {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Engine::Engine(const EngineConfig& config)
    : config_(config) // TODO: validate config?
{
    try {
        setup_subsystems_();

        if (config.enable_graphics)
            setup_graphics_subsystems_();
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Failed to initialize subsystem: ") + e.what()
        );
    }

    setup_events_();

    context_ = {
        .engine     = this,
        .clock      = clock_     .get(),
        .event_bus  = event_bus_ .get(),
        .event_pump = event_pump_.get(),
        .input      = input_     .get(),
        .logger     = logger_    .get(),
        .random     = random_    .get(),
        .stopwatch  = stopwatch_ .get(),
        .device     = device_    .get(),
        .window     = window_    .get(),
        .render     = render_    .get(),
        .renderer2d = renderer2d_.get(),
        .ui         = ui_        .get(),
    };

    logger_->info("Constructed Nezumi Engine (v{})", Nezumi::VersionString);
}

Engine::~Engine() {
    if (on_exit_  ) event_bus_->unsubscribe(on_exit_  );
    if (on_escape_) event_bus_->unsubscribe(on_escape_);

    if (input_ && ui_) input_->set_capture(nullptr);
    if (ui_        ) ui_        .reset();
    if (renderer2d_) renderer2d_.reset();
    if (render_    ) render_    .reset();
    if (device_    ) device_    .reset();
    if (window_    ) window_    .reset();
    if (stopwatch_ ) stopwatch_ .reset();
    if (clock_     ) clock_     .reset();
    if (random_    ) random_    .reset();
    if (input_     ) input_     .reset();
    if (event_pump_) event_pump_.reset();
    if (event_bus_ ) event_bus_ .reset();
    if (logger_    ) logger_    .reset();
}

// -------------------------------------------------------------------------
// Setup
// -------------------------------------------------------------------------

void Engine::setup_subsystems_() {

    // Create Logger
    logger_ = std::make_unique<Logger>(config_.logger);
    if (!logger_)
        throw std::runtime_error("Failed to create Logger");

    // Create EventBus
    event_bus_ = std::make_unique<EventBus>(); // TODO: revert back to EventBus
    if (!event_bus_)
        throw std::runtime_error("Failed to create EventBus");

    // Create EventPump
    event_pump_ = std::make_unique<EventPump>(*event_bus_);
    if (!event_pump_)
        throw std::runtime_error("Failed to create EventPump");

    // Create InputMap
    input_ = std::make_unique<InputMap>(*event_bus_);
    if (!input_)
        throw std::runtime_error("Failed to create InputMap");

    // Create Random
    random_ = std::make_unique<Random>(config_.random);
    if (!random_)
        throw std::runtime_error("Failed to create Random");

    // Create Clock
    clock_ = std::make_unique<Clock>(config_.clock);
    if (!clock_)
        throw std::runtime_error("Failed to create Clock");

    // Create Stopwatch
    stopwatch_ = std::make_unique<Stopwatch>();
    if (!stopwatch_)
        throw std::runtime_error("Failed to create Stopwatch");

    // Create Stopwatch for collecting LoopStats
    statswatch_ = std::make_unique<Stopwatch>();
    if (!statswatch_)
        throw std::runtime_error("Failed to create Stopwatch");
}

void Engine::setup_graphics_subsystems_() {

    // Create window prepare function based on device config
    WindowPrepareFunc prepare = [
        backend      = config_.device.backend,
        enable_debug = config_.enable_debug
    ] {
        return RHI::prepare_sdl_window(backend, enable_debug);
    };

    // Create Window
    window_ = std::make_unique<Window>(
        /*.events  = */*event_bus_,
        /*.prepare = */prepare,
        /*.config  = */config_.window
    );
    if (!window_) {
        throw std::runtime_error("Failed to create Window");
    }

    // Create RHI::Device
    device_ = RHI::make_device(
        /*.window = */window_->get_sdl_window(),
        /*.logger = */logger_.get(),
        /*.config = */config_.device
    );
    if (!device_) {
        throw std::runtime_error("Failed to create RHI::Device");
    }

    // Create RenderCore: shared render services, and owner of the frame
    render_ = std::make_unique<Render::RenderCore>(
        /*.device = */*device_,
        /*.config = */config_.render
    );

    // Default recipe: one pass that clears the backbuffer. Renderer2D (when
    // enabled) records every view into it. Games can replace the recipe with
    // render()->set_recipe().
    Render::PassConfig main_pass = {
        .name        = "main",
        .clear       = true,
        .clear_color = config_.render.clear_color,
        .features    = {},
    };

    if (config_.enable_renderer2d) {

        // Create Renderer2D (registers itself with render_)
        renderer2d_ = std::make_unique<Render::Renderer2D>(
            /*.core   = */*render_,
            /*.config = */config_.renderer2d
        );
        main_pass.features.push_back({ .feature = renderer2d_.get() });

        // Create UI, and let it claim the mouse and keyboard from game actions
        // while the player uses it
        ui_ = std::make_unique<UI::Context>(
            /*.events   = */*event_bus_,
            /*.renderer = */*renderer2d_,
            /*.window   = */window_.get(),
            /*.config   = */config_.ui
        );
        input_->set_capture(ui_.get());
    }

    render_->set_recipe({ .passes = { std::move(main_pass) } });
}

void Engine::setup_events_() {

    // Register exit event
    on_exit_ = event_bus_->subscribe<OnGameExited>(
        [this](const OnGameExited&) {
            this->is_running_ = false;
        }
    );

    // Register escape event: pressing escape key exits
    if (config_.escape_exits) {
        on_escape_ = event_bus_->subscribe<OnKeyDown>(
            [this](const OnKeyDown& e) {
                if (e.scancode == ScanCode::Escape) this->is_running_ = false;
            }
        );
    }
}

// -----------------------------------------------------------------------------
// Control
// -----------------------------------------------------------------------------

void Engine::run_init_(IApplication& app) {
    app.on_init(*this);
    event_bus_->dispatch(OnGameInitialized{});
}

void Engine::run_tick_(IApplication& app) {
    statswatch_->start();

    // TODO: app.on_tick_pre(*this);
    app.on_tick(*this);

    // on_tick has seen this tick's presses and releases
    input_->clear_edges();

    // TODO: app.on_tick_post(*this);

    event_bus_->flush(EventPhase::TickPost, clock_->get_tick());

    event_bus_->dispatch(OnGameTicked{});

    stats_.tick_ms = statswatch_->get_ms();
    stats_.tick_count++;
}

void Engine::run_draw_(IApplication& app) {
    statswatch_->start();

    // HiDPI: draw at the screen's full resolution, scaled to keep sizes
    if (render_ && window_) render_->set_display_scale(window_->get_display_scale());
    if (render_) render_->begin_frame();
    if (ui_) ui_->begin_frame();
    app.on_draw(*this);
    if (ui_) ui_->end_frame();
    if (render_) render_->end_frame();
    event_bus_->dispatch(OnGameDrawn{});

    stats_.draw_ms = statswatch_->get_ms();
    stats_.draw_count++;
}

void Engine::run_exit_(IApplication& app) {
    app.on_exit(*this);
    event_bus_->dispatch(OnGameExited{});
}

int Engine::run(IApplication& app) {
    // TODO: handle engine pauses effectively

    run_init_(app);

    is_running_ = true;
    clock_->reset();

    while (is_running_) {
        clock_->begin(!is_paused_);
        event_pump_->poll_events();

        bool ticked = false;
        while (clock_->should_update()) {
            run_tick_(app);
            ticked = true;
        }

        if (config_.enable_graphics || ticked) {
            run_draw_(app);
        }

        clock_->wait_until_next_tick();
    }

    run_exit_(app);
    return 0; // Success
}

} // namespace Nezumi
