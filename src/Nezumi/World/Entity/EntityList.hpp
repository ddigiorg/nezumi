// =============================================================================
// EntityList.hpp
//
// Circular intrusive doubly-linked list for storing entity object pointers.
// =============================================================================

#pragma once
#include "Nezumi/Core/IntrusiveList.hpp"
#include "Nezumi/Core/IntrusiveListNode.hpp"

namespace Nezumi {

class Entity;

using EntityListNode = IntrusiveListNode<Entity>;
using EntityList     = IntrusiveList    <Entity>;

} // namespace Nezumi
