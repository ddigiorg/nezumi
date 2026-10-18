// =============================================================================
// EventListenerID.hpp
//
// Listener identification.
// =============================================================================

#pragma once
#include "Nezumi/Object/ID.hpp"

namespace Nezumi {

struct IEvent;

using EventListenerID = ID<IEvent>;

} // namespace Nezumi
