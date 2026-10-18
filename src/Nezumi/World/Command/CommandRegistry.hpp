// =============================================================================
// CommandRegistry.hpp
// =============================================================================

#pragma once
#include "Nezumi/World/Command/DeferEntityCreate.hpp"
#include "Nezumi/World/Command/DeferEntityRemove.hpp"
#include "Nezumi/World/Command/DeferEntitySetActive.hpp"
#include <vector>  // vector

namespace Nezumi {

class CommandRegistry {
public:

    // -------------------------------------------------------------------------
    // Defer
    // -------------------------------------------------------------------------

    void defer_entity_create(const DeferEntityCreate& cmd) {
        entity_creates_.push_back(cmd);
    }

    void defer_entity_remove(const DeferEntityRemove& cmd) {
        entity_removes_.push_back(cmd);
    }

    void defer_entity_set_active(const DeferEntitySetActive& cmd) {
        entity_set_actives_.push_back(cmd);
    }

    // -------------------------------------------------------------------------
    // Apply
    //
    // Buffers are swapped out before processing, so commands issued while
    // applying (e.g. from an entity constructor or destructor) land in the next
    // command phase instead of mutating the buffer being walked.
    // -------------------------------------------------------------------------

    template <typename Fn>
    void apply_entity_creates(Fn&& fn) {
        auto cmds = entity_creates_;
        entity_creates_ = {};
        for (const auto& cmd : cmds) fn(cmd);
    }

    template <typename Fn>
    void apply_entity_removes(Fn&& fn) {
        auto cmds = entity_removes_;
        entity_removes_ = {};
        for (const auto& cmd : cmds) fn(cmd);
    }

    template <typename Fn>
    void apply_entity_set_actives(Fn&& fn) {
        auto cmds = entity_set_actives_;
        entity_set_actives_ = {};
        for (const auto& cmd : cmds) fn(cmd);
    }

private:

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::vector<DeferEntityCreate>    entity_creates_     = {};
    std::vector<DeferEntityRemove>    entity_removes_     = {};
    std::vector<DeferEntitySetActive> entity_set_actives_ = {};
    // std::vector<DamageCommand_>   pending_damage_      = {}; // TODO
};

} // namespace Nezumi
