// =============================================================================
// SlotMap.hpp
//
// A handle-addressed container of heap-allocated objects.
//
// - Objects are individually heap-allocated, a T* is stable for its lifetime
// - Freed slots are recycled through an index-based intrusive free list.
// - Each slot carries a version counter, so a handle to a destroyed object
//   resolves to nullptr rather than aliasing its replacement.
// - Supports polymorphic storage: SlotMap<Entity> can hold any Entity subclass.
// - The handle type can differ from the stored type: SlotMap<T, Tag> stores T
//   but hands out Handle<Tag>. Tag defaults to T. Used where a public handle
//   names an abstract resource and a private implementation stores the concrete
//   data, e.g. SlotMap<GLTexture, RHI::Texture> hands out Handle<RHI::Texture>.
//
// Not thread-safe.
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"
#include <cassert>     // assert
#include <cstddef>     // size_t
#include <memory>      // unique_ptr
#include <type_traits> // is_base_of_v, is_same_v, has_virtual_destructor_v
#include <utility>     // as_const, forward, move
#include <vector>      // vector

namespace Nezumi {

template <typename T, typename Tag = T>
class SlotMap {
public:

    // -------------------------------------------------------------------------
    // Types
    // -------------------------------------------------------------------------

    using Type    = T;
    using Index   = typename Handle<Tag>::Index;
    using Version = typename Handle<Tag>::Version;

    static constexpr Index   NULL_INDEX   = Handle<Tag>::NULL_INDEX;
    static constexpr Version NULL_VERSION = Handle<Tag>::NULL_VERSION;

    // -------------------------------------------------------------------------
    // Pair
    //
    // Returned by create() or emplace() and contains both handle and object.
    // -------------------------------------------------------------------------

    struct Pair {
        Handle<Tag> handle = {};
        T*          object = nullptr;

        explicit operator bool() const noexcept { return object != nullptr; }
    };

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    SlotMap()  = default;
    ~SlotMap() = default;

    // Non-copyable and non-movable
    SlotMap(const SlotMap&)            = delete;
    SlotMap& operator=(const SlotMap&) = delete;
    SlotMap(SlotMap&&)                 = delete;
    SlotMap& operator=(SlotMap&&)      = delete;

    // -------------------------------------------------------------------------
    // Reserve
    //
    // Reserve capacity for n slots. Does not create slots or objects.
    // -------------------------------------------------------------------------

    void reserve(std::size_t n) {
        slots_.reserve(n);
    }

    // -------------------------------------------------------------------------
    // Clear
    //
    // Destroys every live object and recycles every slot. Slot storage and
    // version history are retained, so ids minted before the clear stay dead.
    // -------------------------------------------------------------------------

    void clear() {
        for (Index i = 0; i < static_cast<Index>(slots_.size()); i++) {
            if (slots_[i].object) slot_release_(i);
        }
        assert(count_ == 0);
    }

    // -------------------------------------------------------------------------
    // Create
    //
    // Construct an object in a recycled or newly appended slot. D defaults to T
    // for the non-polymorphic case; pass D explicitly to build a derived type.
    //
    //   SlotMap<Entity> entities;
    //   entities.create(config);       // Entity (T)
    //   entities.create<Unit>(config); // Unit (D), stored as Entity (T)
    // -------------------------------------------------------------------------

    template <typename D = T, typename... Args>
    Pair create(Args&&... args) {
        static_assert(
            std::is_base_of_v<T, D>,
            "SlotMap<T>::create<D>: D must derive from T");

        static_assert(
            std::is_same_v<D, T> || std::has_virtual_destructor_v<T>,
            "SlotMap<T>: T needs a virtual destructor to own derived objects");

        return emplace(std::make_unique<D>(std::forward<Args>(args)...));
    }

    // -------------------------------------------------------------------------
    // Emplace
    //
    // Take ownership of an already-constructed object. Useful when a save
    // loader or type factory hands back a unique_ptr<T>.
    // -------------------------------------------------------------------------

    Pair emplace(std::unique_ptr<T> object) {
        if (!object) return {};

        const Index index = slot_acquire_();
        Slot_&      slot  = slots_[index];

        slot.object = std::move(object);
        count_++;

        return Pair{ Handle<Tag>(index, slot.version), slot.object.get() };
    }

    // -------------------------------------------------------------------------
    // Remove
    //
    // Destroy an object and recycle its slot. A stale or null handle is a
    // no-op, so double-removal is safe.
    // -------------------------------------------------------------------------

    void remove(Handle<Tag> handle) {
        if (!slot_resolve_(handle)) return;
        slot_release_(handle.index());
    }

    // -------------------------------------------------------------------------
    // Get
    //
    // Returns the object or nullptr for a null, out-of-range or stale handle.
    // -------------------------------------------------------------------------

    T* get(Handle<Tag> handle) {
        Slot_* slot = slot_resolve_(handle);
        return slot ? slot->object.get() : nullptr;
    }

    const T* get(Handle<Tag> handle) const {
        const Slot_* slot = slot_resolve_(handle);
        return slot ? slot->object.get() : nullptr;
    }

    // Checked downcast. Returns nullptr if the handle is dead OR the object is
    // not a D. Requires T to be polymorphic.
    template <typename D>
    D* get_as(Handle<Tag> handle) {
        static_assert(
            std::is_base_of_v<T, D>,
            "SlotMap<T>::get_as<D>: D must derive from T");

        return dynamic_cast<D*>(get(handle));
    }

    bool is_alive(Handle<Tag> handle) const {
        return slot_resolve_(handle) != nullptr;
    }

    // -------------------------------------------------------------------------
    // Get Handle
    //
    // Recover handle from slot index.
    // -------------------------------------------------------------------------

    Handle<Tag> get_handle(Index index) const {
        if (index >= slots_.size()) return {};
        const Slot_& slot = slots_[index];
        if (!slot.object) return {};
        return Handle<Tag>(index, slot.version);
    }

    // -------------------------------------------------------------------------
    // Get By Index
    //
    // Returns the object at an index (ignoring version)
    // -------------------------------------------------------------------------

    // TODO: double check these
    T* get_by_index(Index index) {
        if (index >= slots_.size()) return nullptr;
        Slot_& slot = slots_[index];
        if (!slot.object) return nullptr;
        return slot.object.get();
    }

    const T* get_by_index(Index index) const {
        if (index >= slots_.size()) return nullptr;
        const Slot_& slot = slots_[index];
        if (!slot.object) return nullptr;
        return slot.object.get();
    }

    // -------------------------------------------------------------------------
    // Iteration
    //
    // Visits every live object. Objects created during iteration are appended
    // and WILL be visited in the same pass; objects removed during iteration
    // are skipped if not yet reached. Prefer a deferred-removal queue in the
    // owning system over removing mid-iteration.
    // -------------------------------------------------------------------------

    template <typename F>
    void for_each(F&& fn) {
        for (Index i = 0; i < static_cast<Index>(slots_.size()); i++) {
            if (T* object = slots_[i].object.get()) {
                fn(*object);
            }
        }
    }

    template <typename F>
    void for_each(F&& fn) const {
        for (Index i = 0; i < static_cast<Index>(slots_.size()); i++) {
            if (const T* object = slots_[i].object.get()) {
                fn(std::as_const(*object));
            }
        }
    }

    // Same, but also hands the handle to the callback: fn(Handle, T&).
    template <typename F>
    void for_each_handle(F&& fn) {
        for (Index i = 0; i < static_cast<Index>(slots_.size()); i++) {
            Slot_& slot = slots_[i];
            if (slot.object) {
                fn(Handle<Tag>(i, slot.version), *slot.object);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    bool   empty()       const { return count_ == 0;   }
    size_t count()       const { return count_;        }
    size_t capacity()    const { return slots_.size(); }
    size_t sizeof_type() const { return sizeof(T);     }
    size_t sizeof_slot() const { return sizeof(Slot_); }

private:

    // -------------------------------------------------------------------------
    // Slot
    //
    // object == nullptr is the free flag and next_free is meaningful only while
    // the slot is free.
    // -------------------------------------------------------------------------

    struct Slot_ {
        std::unique_ptr<T> object;                 // 8 B, null => free
        Version            version   = 1;          // 4 B, 0 is reserved
        Index              next_free = NULL_INDEX; // 4 B
    };

    // -------------------------------------------------------------------------
    // Slot Acquire
    //
    // Pops the free list, or appends a fresh slot. Returns its index.
    // -------------------------------------------------------------------------

    Index slot_acquire_() {
        if (free_head_ != NULL_INDEX) {
            const Index index = free_head_;
            free_head_ = slots_[index].next_free;
            slots_[index].next_free = NULL_INDEX;
            return index;
        }

        assert(
            slots_.size() < NULL_INDEX &&
            "SlotMap: slot index space exhausted");

        slots_.emplace_back();
        return static_cast<Index>(slots_.size() - 1);
    }

    // -------------------------------------------------------------------------
    // Slot Release
    //
    // The object is moved out and destroyed last after the slot is fully
    // recycled. This way a ~T() that removes other objects (and may therefore
    // reallocate slots_) sees a consistent container and cannot dangle any
    // reference we still hold.
    // -------------------------------------------------------------------------

    void slot_release_(Index index) {
        Slot_& slot = slots_[index];
        assert(slot.object && "SlotMap: releasing a slot that is already free");

        std::unique_ptr<T> dying = std::move(slot.object);

        slot.version++;
        if (slot.version == NULL_VERSION) slot.version = 1; // skip 0 on wrap

        slot.next_free = free_head_;
        free_head_     = index;
        count_--;

        // dying destructs here.
    }

    // -------------------------------------------------------------------------
    // Slot Resolve
    // -------------------------------------------------------------------------

    const Slot_* slot_resolve_(Handle<Tag> handle) const {
        if (!handle) return nullptr;
        if (handle.index() >= slots_.size()) return nullptr;

        const Slot_& slot = slots_[handle.index()];

        if (!slot.object) return nullptr;
        if (slot.version != handle.version()) return nullptr;

        return &slot;
    }

    Slot_* slot_resolve_(Handle<Tag> handle) {
        return const_cast<Slot_*>(std::as_const(*this).slot_resolve_(handle));
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::vector<Slot_> slots_;

    size_t count_     = 0;
    Index  free_head_ = NULL_INDEX;
};

} // namespace Nezumi
