#pragma once

#include "ecs/chunk_list.hpp"
#include "ecs/chunk.hpp"

#include <vector>

namespace Game::ECS {

// =============================================================================
// QueryIterator
// =============================================================================

class QueryIterator {
public:
    QueryIterator(const std::vector<ChunkList*>& lists, bool is_end = false)
            : lists_(lists), list_idx_(0), current_chunk_(nullptr) {
        if (!is_end) advance_();
    }

    Chunk* operator*()  const { return current_chunk_; }
    Chunk* operator->() const { return current_chunk_; }

    QueryIterator& operator++() {
        advance_();
        return *this;
    }

    bool operator!=(const QueryIterator& other) const {
        return current_chunk_ != other.current_chunk_;
    }

    explicit operator bool() const { return current_chunk_ != nullptr; }

private:
    void advance_();

    const std::vector<ChunkList*>& lists_;
    size_t list_idx_;
    Chunk* current_chunk_;
};

void QueryIterator::advance_() {

    // step to next chunk in current list
    if (current_chunk_) {
        current_chunk_ = current_chunk_->nextChunk();
        if (current_chunk_)
            return;
        list_idx_++;
    }

    // get head chunk from next list
    while (list_idx_ < lists_.size()) {
        ChunkList* list = lists_[list_idx_];
        if (list) {
            Chunk* head_chunk = list->headChunk();
            if (head_chunk) {
                current_chunk_ = head_chunk;
                return;
            }
        }
        list_idx_++;
    }

    // end of iteration
    current_chunk_ = nullptr;
}

// =============================================================================
// Query
// =============================================================================

class Query {
    friend class QueryManager;

public:
    Query(ComponentMask mask = 0) : mask_(mask) {}

    // range-based for loop support
    QueryIterator begin() const { return QueryIterator(matched_lists_, false); }
    QueryIterator end()   const { return QueryIterator(matched_lists_, true);  }

private:
    ComponentMask mask_;
    std::vector<ChunkList*> matched_lists_;
};

// =============================================================================
// QueryManager
// =============================================================================

class QueryManager {
public:
    QueryManager(ChunkManager& chunk_mgr) : chunk_mgr_(chunk_mgr) {}

    Query query(ComponentMask mask);

private:
    void _findMatchingChunkLists(Query& query);

    ChunkManager& chunk_mgr_;
    std::unordered_map<ComponentMask, Query> cache_; // TODO: implement
};

Query QueryManager::query(ComponentMask mask) {
    // NOTE: Query caching disabled because it breaks when new archetypes are created.
    // Cached queries don't include newly created chunk lists_ for new archetypes.
    // This needs archetype invalidation to work correctly.

    // Create fresh query every time
    Query new_query(mask);
    new_query.matched_lists_ = chunk_mgr_.getMatchedLists(mask);
    return new_query;
}

} // namespace Game::ECS