// =============================================================================
// RingBuffer.hpp
//
// A fixed-capacity FIFO that never reallocates after construction: pushing
// into a full buffer overwrites the oldest element. Index 0 is the oldest,
// size() - 1 the newest.
//
//   RingBuffer<Particle> particles(4096);
//   particles.push(p);                       // drops the oldest when full
//   particles.for_each([](Particle& p) { ... });
//   while (!particles.empty() && particles.front().dead()) particles.pop_front();
//
// Good for things that are fine to lose when there are too many of them:
// particles, decals, recent log lines, a rolling window of frame times.
//
// T must be default-constructible and copy- or move-assignable. Not
// thread-safe.
// =============================================================================

#pragma once
#include <cassert> // assert
#include <cstddef> // size_t
#include <utility> // move
#include <vector>  // vector

namespace Nezumi {

template <typename T>
class RingBuffer {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit RingBuffer(std::size_t capacity)
        : slots_(capacity)
    {
        assert(capacity > 0 && "RingBuffer capacity must be positive");
    }

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    std::size_t capacity() const { return slots_.size(); }
    std::size_t size()     const { return size_;         }
    bool        empty()    const { return size_ == 0;    }
    bool        full()     const { return size_ == slots_.size(); }

    // -------------------------------------------------------------------------
    // Access (0 is the oldest)
    // -------------------------------------------------------------------------

    T& operator[](std::size_t i) {
        assert(i < size_ && "RingBuffer index out of range");
        return slots_[wrap_(head_ + i)];
    }

    const T& operator[](std::size_t i) const {
        assert(i < size_ && "RingBuffer index out of range");
        return slots_[wrap_(head_ + i)];
    }

    T&       front()       { return (*this)[0];         }
    const T& front() const { return (*this)[0];         }
    T&       back()        { return (*this)[size_ - 1]; }
    const T& back()  const { return (*this)[size_ - 1]; }

    // -------------------------------------------------------------------------
    // Modifiers
    // -------------------------------------------------------------------------

    // Append `value` as the newest element. When full, it replaces the
    // oldest. Returns the stored element.
    T& push(T value) {
        T& slot = slots_[wrap_(head_ + size_)];
        slot = std::move(value);

        if (full()) head_ = wrap_(head_ + 1); // overwrote the oldest
        else        size_++;
        return slot;
    }

    // Drop the oldest element (its slot keeps the old value until reused)
    void pop_front() {
        assert(size_ > 0 && "RingBuffer is empty");
        head_ = wrap_(head_ + 1);
        size_--;
    }

    void clear() {
        head_ = 0;
        size_ = 0;
    }

    // -------------------------------------------------------------------------
    // Iteration (oldest to newest)
    // -------------------------------------------------------------------------

    template <typename Fn>
    void for_each(Fn&& fn) {
        for (std::size_t i = 0; i < size_; i++) fn(slots_[wrap_(head_ + i)]);
    }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (std::size_t i = 0; i < size_; i++) fn(slots_[wrap_(head_ + i)]);
    }

private:
    std::size_t wrap_(std::size_t i) const {
        return i >= slots_.size() ? i - slots_.size() : i;
    }

    std::vector<T> slots_;
    std::size_t    head_ = 0; // slot of the oldest element
    std::size_t    size_ = 0;
};

} // namespace Nezumi
