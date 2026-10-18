// =============================================================================
// EventPump.hpp
//
// Drains the SDL event queue each frame and translates raw SDL events into
// typed game events.
//
// Reference: https://wiki.libsdl.org/SDL3/SDL_EventType
// =============================================================================

#pragma once
#include "Nezumi/Event/EventBus.hpp"

namespace Nezumi {

class EventPump {
public:
    EventPump(EventBus& events) : events_(events) {};
    void poll_events();

private:
    EventBus& events_;
};

} // namespace Nezumi
