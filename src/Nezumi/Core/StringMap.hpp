// =============================================================================
// StringMap.hpp
//
// A string-keyed map with allocation-free lookup by string_view.
//
// Owns its keys as std::string; looks up by std::string_view without
// constructing a temporary. Thin wrapper over std::unordered_map that exists
// mainly to keep the transparent-hash boilerplate in one place.
//
// Not thread-safe.
// =============================================================================

#pragma once
#include "Nezumi/Core/StringHash.hpp"
#include <cstddef>       // size_t
#include <functional>    // equal_to
#include <string>        // string
#include <string_view>   // string_view
#include <unordered_map> // unordered_map
#include <utility>       // forward, move

namespace Nezumi {

template <typename T>
class StringMap {
public:

    // -------------------------------------------------------------------------
    // Types
    // -------------------------------------------------------------------------

    using Type = T;
    using Map  = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

    using iterator       = typename Map::iterator;
    using const_iterator = typename Map::const_iterator;

    // -------------------------------------------------------------------------
    // Emplace
    //
    // Construct a value under the given name. Returns nullptr if the name is
    // already taken; in that case `name` is NOT moved from (try_emplace only
    // consumes its arguments on successful insertion).
    // -------------------------------------------------------------------------

    template <typename... Args>
    T* emplace(std::string name, Args&&... args) {
        auto [it, inserted] = map_.try_emplace(
            std::move(name),
            std::forward<Args>(args)...
        );
        return inserted ? &it->second : nullptr;
    }

    // -------------------------------------------------------------------------
    // Find
    //
    // Returns nullptr if absent. The returned pointer is stable until the entry
    // is erased (unordered_map nodes do not move on rehash).
    // -------------------------------------------------------------------------

    T* find(std::string_view name) {
        auto it = map_.find(name);
        return it != map_.end() ? &it->second : nullptr;
    }

    const T* find(std::string_view name) const {
        auto it = map_.find(name);
        return it != map_.end() ? &it->second : nullptr;
    }

    bool contains(std::string_view name) const {
        return map_.find(name) != map_.end();
    }

    // -------------------------------------------------------------------------
    // Erase
    // -------------------------------------------------------------------------

    bool erase(std::string_view name) {
        auto it = map_.find(name);
        if (it == map_.end()) return false;
        map_.erase(it);
        return true;
    }

    void clear()                { map_.clear();    }
    void reserve(std::size_t n) { map_.reserve(n); }

    // -------------------------------------------------------------------------
    // Iteration (yields std::pair<const std::string, T>)
    // -------------------------------------------------------------------------

    iterator       begin()       { return map_.begin(); }
    iterator       end()         { return map_.end();   }
    const_iterator begin() const { return map_.begin(); }
    const_iterator end()   const { return map_.end();   }

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    bool        empty() const { return map_.empty(); }
    std::size_t count() const { return map_.size();  }

private:
    Map map_;
};

} // namespace Nezumi
