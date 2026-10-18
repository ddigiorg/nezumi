// =============================================================================
// IntrusiveList.hpp
//
// Circular intrusive doubly linked list keyed on a Tag type.  No thread safety.
// =============================================================================

#pragma once
#include "Nezumi/Core/IntrusiveListNode.hpp"
#include <cassert>
#include <cstddef>

namespace Nezumi {

template<typename Tag>
class IntrusiveList {
public:
    using Node = IntrusiveListNode<Tag>;

    IntrusiveList() {
        sentinel_.prev = &sentinel_;
        sentinel_.next = &sentinel_;
    }

    // Non-copyable
    IntrusiveList(const IntrusiveList&)            = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    // -------------------------------------------------------------------------
    // Mutation
    // -------------------------------------------------------------------------

    void insert(Node* node) {
        assert(!node->prev && "Node must exit list before reinserting");
        assert(!node->next && "Node must exit list before reinserting");

        // Inserts node at the back of the list
        Node* tail = sentinel_.prev;
        node->next = &sentinel_;
        node->prev = tail;
        tail->next = node;
        sentinel_.prev = node;

        count_++;
    }

    void remove(Node* node) {
        if (!node->prev || !node->next) return;

        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev       = nullptr;
        node->next       = nullptr;

        count_--;
    }

    // -------------------------------------------------------------------------
    // Iteration
    //
    // Example hot loop iteration (manual):
    //
    //   auto* node = list.begin();
    //   while (node != list.end()) {
    //       auto* next = node->next;
    //       static_cast<Type*>(node)->func();
    //       node = next;
    //   }
    //
    // Example hot loop iteration (typed member):
    //
    //   list.each([&](IntrusiveListNode<Tag>* node) {
    //       static_cast<Type*>(node)->func();
    //   });
    // -------------------------------------------------------------------------

    Node* begin() const { return sentinel_.next;                }
    Node* end()   const { return const_cast<Node*>(&sentinel_); }

    // TODO: add prefetch functionality when ready
    // struct PrefetchHint {
    //     int lines    = 0; // Number of cache lines to prefetch (0 = disabled)
    //     int rw       = 0; // Read or write intent (0 = read, 1 = write)
    //     int locality = 3; // Temporal locality (0 = None, 1 = L3, 2 = L2, 3 = L1)
    // };

    template<typename Fn>
    void each(Fn&& fn/*, PrefetchHint hint = {}*/) {
        Node* node = begin();
        while (node != end()) {
            Node* next = node->next;
            // for (int i = 0; i < hint.lines; ++i) {
            //     __builtin_prefetch(
            //         /*.addr     = */reinterpret_cast<char*>(next) + i * 64, // TODO: static_cast<char*>(static_cast<void*>(next))
            //         /*.rw       = */hint.rw,
            //         /*.locality = */hint.locality);
            // }
            fn(node);
            node = next;
        }
    }

    template<typename Fn>
    void each(Fn&& fn/*, PrefetchHint hint = {}*/) const {
        Node* node = begin();
        // for (int i = 0; i < hint.lines; ++i) {
        //     __builtin_prefetch(
        //         /*.addr     = */reinterpret_cast<char*>(next) + i * 64,
        //         /*.rw       = */hint.rw,
        //         /*.locality = */hint.locality);
        // }
        while (node != end()) {
            Node* next = node->next;
            fn(node);
            node = next;
        }
    }

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    bool   empty() const { return sentinel_.next == &sentinel_; }
    size_t count() const { return count_; }

private:
    mutable Node sentinel_;
    size_t count_ = 0;
};

} // namespace Nezumi
