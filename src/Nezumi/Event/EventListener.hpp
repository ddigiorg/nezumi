// =============================================================================
// EventListener.hpp
//
// Type-erased callback wrapper. make<E>() captures a typed callable and casts
// the void* back to the concrete event type. Safe because EventBus only ever
// invokes a listener with the event type it was registered for.
//
// A null `callback` marks a tombstone: the listener was unsubscribed while a
// dispatch was in flight. Tombstones are skipped during dispatch and compacted
// away once the outermost dispatch returns.
// =============================================================================

#pragma once
#include "Nezumi/Event/EventListenerID.hpp"
#include <functional> // std::function

namespace Nezumi {

struct EventListener {
    EventListenerID                  id       = {};
    std::function<void(const void*)> callback = {};

    template<typename E, typename F>
    static EventListener make(EventListenerID id, F&& cb) {
        EventListener l;
        l.id       = id;
        l.callback = [cb = std::forward<F>(cb)](const void* e) {
            cb(*static_cast<const E*>(e));
        };
        return l;
    }
};

} // namespace Nezumi
