// =============================================================================
// EventBus.hpp
//
// Publish/subscribe event bus with per-phase tick-scheduled deferred queues.
// Not thread-safe so must confine to the single game thread.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include "Nezumi/Event/EventListener.hpp"
#include "Nezumi/Event/EventListenerID.hpp"
#include "Nezumi/Event/EventPhase.hpp"
#include "Nezumi/Event/IEvent.hpp"
#include <algorithm> // std::find_if
#include <array>
#include <map>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Nezumi {

class EventBus {
public:
    EventBus() = default;

    // Non-copyable and non-movable
    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&)                 = delete;
    EventBus& operator=(EventBus&&)      = delete;

    // -------------------------------------------------------------------------
    // Subscribe
    //
    // Register a callable for events of type E. Accepts any callable with the
    // signature void(const E&): lambda, function pointer, functor.
    //
    // (std::function still erases the type; if profiling ever shows this on a
    // hot path, swap EventListener's storage for an inplace_function<N>. The
    // bus API won't need to change.)
    //
    // Example:
    //
    //   auto id = bus.subscribe<OnKeyDown>([](const OnKeyDown& e) { ... });
    // -------------------------------------------------------------------------

    template<typename E, typename F>
    EventListenerID subscribe(F&& callback) {
        static_assert(std::is_base_of_v<IEvent, E>, "E must derive from IEvent");
        static_assert(!std::is_same_v<IEvent, E>,   "E must derive from IEvent");

        const auto            idx = std::type_index(typeid(E));
        const EventListenerID id  { id_counter_++ };

        listeners_[idx].push_back(
            EventListener::make<E>(id, std::forward<F>(callback))
        );
        id_to_type_.emplace(id, idx);
        return id;
    }

    // -------------------------------------------------------------------------
    // Unsubscribe
    //
    // Remove the listener with the given ID. Safe to call with a null or
    // already-removed ID, and safe to call from inside a listener callback.
    //
    // Example:
    //
    //   bus.unsubscribe(id);
    // -------------------------------------------------------------------------

    void unsubscribe(EventListenerID id) {
        const auto type_it = id_to_type_.find(id);
        if (type_it == id_to_type_.end()) return;

        const auto listeners_it = listeners_.find(type_it->second);

        if (listeners_it != listeners_.end()) {
            auto& vec = listeners_it->second;

            const auto l = std::find_if(
                /*.first = */vec.begin(),
                /*.last  = */vec.end(),
                /*.pred  = */[id](const EventListener& e) { return e.id == id; }
            );

            if (l != vec.end()) {
                if (dispatch_depth_ > 0u) {
                    l->callback  = nullptr; // tombstone skipped mid-dispatch
                    needs_compact_ = true;
                } else {
                    vec.erase(l);
                }
            }
        }

        id_to_type_.erase(type_it);
    }

    // -------------------------------------------------------------------------
    // Dispatch
    //
    // Immediately invoke all matching listeners for event E.
    //
    // Iteration is index-based over a size snapshot, so push_back from
    // subscribe() (even under reallocation) and tombstoning from unsubscribe()
    // are both safe mid-loop.
    //
    // Example:
    //
    //   bus.dispatch(OnKeyDown{ .key = KeyCode::W }); // fire now
    // -------------------------------------------------------------------------

    template<typename E>
    void dispatch(const E& event) {
        static_assert(std::is_base_of_v<IEvent, E>, "E must derive from IEvent");
        static_assert(!std::is_same_v<IEvent, E>,   "E must derive from IEvent");

        const auto it = listeners_.find(std::type_index(typeid(E)));
        if (it == listeners_.end()) return;

        auto& vec = it->second;

        dispatch_depth_++;
        const size_t count = vec.size(); // listeners added during dispatch wait
        for (size_t i = 0; i < count; i++) {
            if (!vec[i].callback) continue; // skip tombstones
            vec[i].callback(static_cast<const void*>(&event));
        }
        dispatch_depth_--;

        if (dispatch_depth_ == 0 && needs_compact_)
            compact_();
    }

    // -------------------------------------------------------------------------
    // Enqueue
    //
    // Schedule an event into a phase's tick-keyed queue. Dispatches during
    // flush(phase, current_tick) once current_tick >= tick.
    //
    // tick defaults to 0 == "next flush of this phase".
    //
    // The concrete type is captured here at compile time via invoke_thunk_,
    // so flush() needs no typeid(*event) vtable lookup.
    //
    // Insert is O(log D) where D is the number of distinct scheduled ticks in
    // this phase (tree descent to the bucket), then amortized O(1) push_back.
    //
    // Example:
    //
    //   bus.enqueue(
    //       /*.event = */OnTrySpawnEntity{ .config = BLUE_UNIT },
    //       /*.phase = */Phase::TickPost,
    //       /*.tick  = */3600u // fire at tick 3600 (60s @60UPS)
    //    );
    // -------------------------------------------------------------------------

    template<typename E>
    void enqueue(E event, EventPhase phase = EventPhase::TickPost, Tick tick = 0u) {
        static_assert(std::is_base_of_v<IEvent, E>, "E must derive from IEvent");
        static_assert(!std::is_same_v<IEvent, E>,   "E must derive from IEvent");

        auto& q = queues_[get_phase_index(phase)];
        q[tick].push_back(QueuedEvent_{
            .event  = EventPtr_(new E(std::move(event)),
                                [](void* p) { delete static_cast<E*>(p); }),
            .invoke = &invoke_thunk_<E>,
        });
    }

    // -------------------------------------------------------------------------
    // Flush
    //
    // Dispatch every event queued for `phase` whose tick <= current_tick, in
    // (tick, insertion) order. Call at the matching checkpoint in the loop,
    // passing Clock::get_tick().
    //
    // upper_bound(current_tick) is the first bucket strictly after now, so
    // [begin, cutoff) is exactly the due range in ascending tick order, for
    // free. Not-yet-due buckets are never visited.
    //
    // Due events are moved out and the buckets erased BEFORE any listener
    // runs, so events a listener enqueues land back in the map and wait for
    // the next flush.  No iterator invalidation, no same-flush feedback loop.
    //
    // Cost: O(log D) to locate the cutoff + O(k) to drain k due events.
    //
    // Example:
    //
    //   bus.flush(Phase::TickPost, clock.get_tick()); // in game loop
    // -------------------------------------------------------------------------

    void flush(EventPhase phase, Tick current_tick) {
        auto& q = queues_[get_phase_index(phase)];
        if (q.empty()) return;

        const auto cutoff = q.upper_bound(current_tick);
        if (q.begin() == cutoff) return; // soonest bucket isn't due yet

        // Move due buckets out (ascending tick, insertion order within each),
        // then erase them, so re-entrant enqueue() targets the next flush.
        std::vector<QueuedEvent_> due;
        for (auto it = q.begin(); it != cutoff; ++it) {
            auto& bucket = it->second;
            due.insert(
                /*.position = */due.end(),
                /*.first    = */std::make_move_iterator(bucket.begin()),
                /*.last     = */std::make_move_iterator(bucket.end())
            );
        }
        q.erase(q.begin(), cutoff);

        for (const auto& qe : due)
            qe.invoke(*this, qe.event.get());
    }


    // -------------------------------------------------------------------------
    // Clear
    //
    // Remove all listeners and discard all queued events in every phase.
    // -------------------------------------------------------------------------

    void clear() {
        listeners_.clear();
        id_to_type_.clear();
        for (auto& q : queues_)
            q.clear();
        needs_compact_ = false;
    }


    // -------------------------------------------------------------------------
    // Diagnostics
    // -------------------------------------------------------------------------

    size_t listener_count() const {
        size_t count = 0;
        for (const auto& [_, vec] : listeners_)
            count += vec.size();
        return count;
    }

    size_t queued_event_count(EventPhase phase) const {
        size_t count = 0;
        for (const auto& [_, bucket] : queues_[get_phase_index(phase)])
            count += bucket.size();
        return count;
    }

    size_t queued_event_count() const {
        size_t count = 0;
        for (const auto& q : queues_)
            for (const auto& [_, bucket] : q)
                count += bucket.size();
        return count;
    }

private:

    // -------------------------------------------------------------------------
    // QueuedEvent
    //
    // Type-erased payload + per-type invoke thunk. The scheduled tick is not
    // stored here, it is the map key. Ownership is a unique_ptr<void> with a
    // deleter captured at enqueue time, which keeps Event non-polymorphic
    // (events stay aggregates) and avoids any typeid/vtable lookup in flush().
    // -------------------------------------------------------------------------

    using EventPtr_ = std::unique_ptr<void, void (*)(void*)>;

    struct QueuedEvent_ {
        EventPtr_ event = { nullptr, nullptr };
        void (*invoke)(EventBus&, const void*) = nullptr;
    };

    template<typename E>
    static void invoke_thunk_(EventBus& bus, const void* e) {
        bus.dispatch(*static_cast<const E*>(e));
    }

    // -------------------------------------------------------------------------
    // Compact
    //
    // Erase tombstoned listeners once no dispatch is occuring.
    // -------------------------------------------------------------------------

    void compact_() {
        for (auto& [_, vec] : listeners_) {
            std::erase_if(
                /*.vector = */vec,
                /*.pred   = */[](const EventListener& l) { return !l.callback; }
            );
        }
        needs_compact_ = false;
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::unordered_map<std::type_index, std::vector<EventListener>> listeners_ = {};
    std::unordered_map<EventListenerID, std::type_index> id_to_type_           = {};
    std::array<std::map<Tick, std::vector<QueuedEvent_>>, PHASE_COUNT> queues_ = {};

    uint32_t id_counter_     = 0u;
    uint32_t dispatch_depth_ = 0u;
    bool     needs_compact_  = false;
};

} // namespace Nezumi
