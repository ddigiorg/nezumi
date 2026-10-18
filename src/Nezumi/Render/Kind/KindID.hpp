// =============================================================================
// KindID.hpp
//
// Kind identification.  Each kind gets a small integer the first time it's
// used, which goes into the sort key and indexes the KindRegistry. IDs follow
// first-use order, so they're stable within a run of the program but not across
// builds: never save them.
// =============================================================================

#pragma once
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include <cstdint> // uint8_t

namespace Nezumi::Render {

using KindID = std::uint8_t;

inline KindID get_next_kind_id() {
    static KindID next = 0;
    return next++;
}

template <KindConcept K>
KindID get_kind_id() {
    static const KindID id = get_next_kind_id();
    return id;
}

} // namespace Nezumi::Render
