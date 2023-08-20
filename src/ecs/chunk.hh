#pragma once

#include "../common/mem.hh"

namespace game {
struct Archetype;
struct Entity;

enum {
  CHUNK_BITS        = 14,
  CHUNK_SIZE        = 1 << 14,             // 16384
  CHUNK_HEADER_SIZE = MEM_CACHE_LINE_SIZE, // cache line
  CHUNK_BUFFER_SIZE = CHUNK_SIZE - CHUNK_HEADER_SIZE,
};

struct alignas(MEM_CACHE_LINE_SIZE) ChunkHeader {
  Archetype* archetype_;
  i32        len_; // entity count
  i32        cap_; // The max number of entities for the chunk
  uint64_t   sequence_number_;
  i32        list_index_;      // The index of the chunk in "archetype chunk data". (allocated)
  i32        free_list_index_; // The index of the chunk in the free list (unallocated)
};

// A chunk is a block of memory (16 KiB). The fist 64 bytes are reserved for the header the remaining laid out in memory as dictated by the archetype.
// All chunks have an entity array that can be accessed regardless of what else may be found in the chunk.
struct Chunk {
  ChunkHeader header_;
  byte        buffer_[1];

  Archetype& Archetype() { return *header_.archetype_; }

  // The number of entities currently allocated in chunk
  int EntityCount() { return header_.len_; }

  void AddEntityCount(int n) { header_.len_ += n; }

  int EntityCapacity() { return header_.cap_; }

  // The index of this chunk in the chunk list
  int ListIndex() { return header_.list_index_; }

  // The index of this chunk in the free list
  int FreeListIndex() { return header_.free_list_index_; }

  // Gets a pointer to the beginning of the usable memory portion of the chunk
  void* Buffer() { return &buffer_[0]; }

  Entity* EntityArray() { return (Entity*)Buffer(); }
};

struct ChunkAllocator {
  // The chunk store is a humongous bitmask.
  // Every time a chunk is to be allocated we search for vacant chunk amongst our allocated mega chunks.
  // We use a word size of 64-bits to represent a mega chunk. 16 KiB * 64 = 1 MiB.
  // Each bit of the word is used to reserve one or more chunks within the mega chunk.
  // It is possible to request more than 1 chunk at a time but we will only ever be able to serve up to 64 in a single request (often less).

  static const int MEGA_CHUNK_SIZE = 64 * CHUNK_SIZE; // A mega chunk is exactly 64 chunks (1 MiB)
  static const int MEGA_CHUNK_MAX  = 16 * 1024;       // Maximum number of mega chunks allowed (16 GiB)

  // mega chunk pointer is stored here
  byte* mega_chunks_[MEGA_CHUNK_MAX];

  // For each chunk that is allocated in a mega chunk the corresponding bit position is set here
  u64 chunk_use_[MEGA_CHUNK_MAX];

  // Allocate a zero initialized chunk
  Chunk* Allocate();

  void Free(Chunk* chunk);

  // Destroy all allocated chunks and free all mega chunks
  void Destroy();
};
} // namespace game