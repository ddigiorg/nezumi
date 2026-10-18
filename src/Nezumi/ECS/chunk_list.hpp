#pragma once

#include "ecs/chunk.hpp"

namespace Game::ECS {

class ChunkList {
public:
    ChunkList(const ComponentMask& mask);
    void insertChunk(Chunk* chunk);
    void removeChunk(Chunk* chunk);
    void insertChunkOpen(Chunk* chunk);
    void removeChunkOpen(Chunk* chunk);
    Chunk* headChunk() noexcept { return head_; }
    Chunk* nextOpenChunk() noexcept { return head_open_; }

private:
    ComponentMask mask_;
    uint32_t count_;
    Chunk* head_;      // list node to head chunk
	Chunk* tail_;      // list node to tail chunk
	Chunk* head_open_; // list node to head chunk with open entity slots
	Chunk* tail_open_; // list node to tail chunk with open entity slots
};

ChunkList::ChunkList(const ComponentMask& mask) {
    mask_ = mask;
    count_ = 0;
    head_ = nullptr;
    tail_ = nullptr;
    head_open_ = nullptr;
    tail_open_ = nullptr;
}

void ChunkList::insertChunk(Chunk* chunk) {
    chunk->next_ = nullptr;
    chunk->prev_ = tail_;

    if (tail_) {
        tail_->next_ = chunk;
    } else {
        head_ = chunk;
    }

    tail_ = chunk;

    count_++;
}

void ChunkList::removeChunk(Chunk* chunk) {
    if (chunk->prev_) {
        chunk->prev_->next_ = chunk->next_;
    } else {
        head_ = chunk->next_;
    }

    if (chunk->next_) {
        chunk->next_->prev_ = chunk->prev_;
    } else {
        tail_ = chunk->prev_;
    }

    chunk->prev_ = nullptr;
    chunk->next_ = nullptr;

    count_--;
}

void ChunkList::insertChunkOpen(Chunk* chunk) {
    chunk->next_open_ = nullptr;
    chunk->prev_open_ = tail_open_;

    if (tail_open_) {
        tail_open_->next_open_ = chunk;
    } else {
        head_open_ = chunk;
    }

    tail_open_ = chunk;
}

void ChunkList::removeChunkOpen(Chunk* chunk) {
    if (chunk->prev_open_) {
        chunk->prev_open_->next_open_ = chunk->next_open_;
    } else {
        head_open_ = chunk->next_open_;
    }

    if (chunk->next_open_) {
        chunk->next_open_->prev_open_ = chunk->prev_open_;
    } else {
        tail_open_ = chunk->prev_open_;
    }

    chunk->prev_open_ = nullptr;
    chunk->next_open_ = nullptr;
}

} // namespace Game::ECS