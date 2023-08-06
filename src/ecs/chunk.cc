#include "chunk.hh"

#include "../common/intrin.hh"

using namespace game;

Chunk* ChunkAllocator::Allocate() {
  for (int i = 0; i < MEGA_CHUNK_MAX; i++) {
    auto use = chunk_use_[i];
    if (use & 0x0000000000000001ULL) {
      continue; // chunk is full (no trailing zero bits)
    } else if (use != 0) {
      uint64_t bit  = tzcnt_u64(use) - 1; // tzcnt_u64(0x8000000000000000ULL) = 63
      auto     mask = 1ULL << bit;
      chunk_use_[i] |= mask;
      return (Chunk*)(mega_chunks_[i] + bit * CHUNK_SIZE);
    } else {
      // use is zero, allocate a new mega chunk and return last chunk (64)
      auto mega_chunk = (uint8_t*)MemAlloc(MEM_ALLOC_HEAP, MEGA_CHUNK_SIZE, MEM_CACHE_LINE_SIZE);
      memset(mega_chunk, 0, size_t(MEGA_CHUNK_SIZE));
      mega_chunks_[i] = mega_chunk;
      chunk_use_[i]   = 0x8000000000000000ULL; // tzcnt_u64(0x0000000000000000ULL) = 64
      return (Chunk*)(mega_chunks_[i] + MEGA_CHUNK_SIZE - CHUNK_SIZE);
    }
  }
  assert(false && "cannot allocate chunk");
  return nullptr;
}

void ChunkAllocator::Free(Chunk* chunk) {
  for (int i = 0; i < MEGA_CHUNK_MAX; i++) {
    auto mega_chunk = mega_chunks_[i];
    if (mega_chunk != nullptr) {
      auto begin = (Chunk*)(mega_chunk);
      auto end   = (Chunk*)(mega_chunk + MEGA_CHUNK_SIZE);
      if ((begin <= chunk) & (chunk <= end)) {
        auto bit  = (uint64_t)((uint8_t*)chunk - mega_chunk) >> CHUNK_BITS;
        auto mask = 1ULL << bit;
        chunk_use_[i] &= ~mask;
        if (chunk_use_[i] == 0) {
          MemFree(MEM_ALLOC_HEAP, mega_chunk);
          mega_chunks_[i] = nullptr;
        } else {
          memset(chunk, 0, CHUNK_SIZE);
        }
        break;
      }
    } else {
      assert(false && "cannot free chunk");
    }
  }
}

void ChunkAllocator::Destroy() {
  for (int i = 0; i < MEGA_CHUNK_MAX; i++) {
    auto mega_chunk = mega_chunks_[i];
    if (mega_chunk != nullptr) {
      MemFree(MEM_ALLOC_HEAP, mega_chunk);
      mega_chunks_[i] = nullptr;
      chunk_use_[i]   = 0;
    } else {
      break;
    }
  }
}