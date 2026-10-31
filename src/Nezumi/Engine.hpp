// =============================================================================
// Engine.hpp
// =============================================================================

#pragma once
#include "Nezumi/Core/Clock.hpp"
#include "Nezumi/Core/EventPump.hpp"
#include "Nezumi/Core/Random.hpp"
#include "Nezumi/Core/Stopwatch.hpp"
#include "Nezumi/Core/Window.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/InputMap.hpp"
#include "Nezumi/Logger/Logger.hpp"
#include "Nezumi/Render/2D/Renderer2D.hpp"
#include "Nezumi/RHI/Device.hpp"
#include "Nezumi/UI/Context.hpp"
#include "Nezumi/World/World.hpp"
#include "Nezumi/EngineConfig.hpp"
#include "Nezumi/EngineContext.hpp"
#include "Nezumi/IApplication.hpp"
#include "Nezumi/Version.hpp"
#include <cstdint>
#include <memory> // unique_ptr

namespace Nezumi {

class IApplication;

class Engine {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    Engine(const EngineConfig& config = {});
    ~Engine();

    // Non-copyable and non-movable
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------

    int run(IApplication& game);

    // TODO
    // void exit();
    // void pause();
    // void unpause();

    // -------------------------------------------------------------------------
    // Loop Statistics
    // -------------------------------------------------------------------------

    struct LoopStats {
        uint64_t tick_count = 0;
        uint64_t draw_count = 0;
        float    tick_ms    = 0.0f;
        float    draw_ms    = 0.0f;
        // float    tps        = 0.0f; // ticks per second
        // float    dps        = 0.0f; // draws per second
        // float    fps        = 0.0f; // frames (tick + draw) per second
    };

    LoopStats get_stats() { return stats_; }

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    bool is_running() const { return is_running_; }
    bool is_paused () const { return is_paused_;  }

    Clock&     clock()      { return *clock_;      }
    EventBus&  event_bus()  { return *event_bus_;  }
    EventPump& event_pump() { return *event_pump_; }
    InputMap&  input()      { return *input_;      }
    Logger&    logger()     { return *logger_;     }
    Random&    random()     { return *random_;     }
    Stopwatch& stopwatch()  { return *stopwatch_;  }
    World&     world()      { return *world_;      }

    Window*           window()   { return window_  .get(); }
    RHI::Device*      device()   { return device_  .get(); }
    Render::Renderer2D* renderer() { return renderer_.get(); }
    UI::Context*      ui()       { return ui_      .get(); }

    EngineContext& context() { return context_; }

private:

    // -------------------------------------------------------------------------
    // Setup
    // -------------------------------------------------------------------------

    void setup_subsystems_();
    void setup_graphics_subsystems_();
    void setup_events_();

    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------

    void run_init_(IApplication& app);
    void run_tick_(IApplication& app);
    void run_draw_(IApplication& app);
    void run_exit_(IApplication& app);

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    const EngineConfig config_;

    EngineContext context_;

    // Subsystems
    std::unique_ptr<Clock    > clock_      = nullptr;
    std::unique_ptr<EventBus > event_bus_  = nullptr;
    std::unique_ptr<EventPump> event_pump_ = nullptr;
    std::unique_ptr<InputMap > input_      = nullptr;
    std::unique_ptr<Logger   > logger_     = nullptr;
    std::unique_ptr<Random   > random_     = nullptr;
    std::unique_ptr<Stopwatch> stopwatch_  = nullptr;
    std::unique_ptr<Stopwatch> statswatch_ = nullptr; // not shared
    std::unique_ptr<World>     world_      = nullptr;

    // Graphics Subsystems (absent if graphics disabled)
    std::unique_ptr<Window          > window_   = nullptr;
    std::unique_ptr<RHI::Device     > device_   = nullptr;
    std::unique_ptr<Render::Renderer2D> renderer_ = nullptr;
    std::unique_ptr<UI::Context     > ui_       = nullptr;

    // Miscellaneous
    EventListenerID on_exit_   = {};
    EventListenerID on_escape_ = {};

    LoopStats stats_ = {};

    bool is_running_ = false;
    bool is_paused_  = false;
};

} //namespace Nezumi
