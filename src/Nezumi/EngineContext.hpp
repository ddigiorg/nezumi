#pragma once

namespace Nezumi {

class Clock;
class Engine;
class EventBus;
class EventPump;
class Logger;
class Random;
class Stopwatch;
class Window;
class World;

namespace Render {
    class Renderer;
}

namespace RHI {
    class Device;
}

class EngineContext {
public:
    Engine* engine = nullptr;

    // Subsystems
    Clock*     clock      = nullptr;
    EventBus*  event_bus  = nullptr;
    EventPump* event_pump = nullptr;
    Logger*    logger     = nullptr;
    Random*    random     = nullptr;
    Stopwatch* stopwatch  = nullptr;
    World*     world      = nullptr;

    // Graphics Subsystems (absent if graphics disabled)
    RHI::Device*      device   = nullptr;
    Window*           window   = nullptr;
    Render::Renderer* renderer = nullptr;
};

} // namespace Nezumi
