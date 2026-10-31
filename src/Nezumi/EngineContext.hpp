#pragma once

namespace Nezumi {

class Clock;
class Engine;
class EventBus;
class EventPump;
class InputMap;
class Logger;
class Random;
class Stopwatch;
class Window;

namespace Render {
    class RenderCore;
    class Renderer2D;
}

namespace RHI {
    class Device;
}

namespace UI {
    class Context;
}

class EngineContext {
public:
    Engine* engine = nullptr;

    // Subsystems
    Clock*     clock      = nullptr;
    EventBus*  event_bus  = nullptr;
    EventPump* event_pump = nullptr;
    InputMap*  input      = nullptr;
    Logger*    logger     = nullptr;
    Random*    random     = nullptr;
    Stopwatch* stopwatch  = nullptr;

    // Graphics Subsystems (absent if graphics disabled)
    RHI::Device*        device     = nullptr;
    Window*             window     = nullptr;
    Render::RenderCore* render     = nullptr;
    Render::Renderer2D* renderer2d = nullptr; // absent if Render2D is disabled
    UI::Context*        ui         = nullptr; // absent if Render2D is disabled
};

} // namespace Nezumi
