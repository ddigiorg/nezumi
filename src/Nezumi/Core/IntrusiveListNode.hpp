// =============================================================================
// IntrusiveListNode.hpp
//
// Typed node base for use in IntrusiveList<Tag>.
//
// Each distinct Tag produces a distinct node type, allowing an object to
// participate in multiple independent intrusive lists simultaneously via
// multiple inheritance.
// =============================================================================

#pragma once

namespace Nezumi {

template<typename Tag>
class IntrusiveListNode {
public:
    IntrusiveListNode* prev = nullptr; // offset within this subobject: 0
    IntrusiveListNode* next = nullptr; // offset within this subobject: 8

    IntrusiveListNode()  = default;
    ~IntrusiveListNode() = default;
};

} // namespace Nezumi
