#pragma once

#include "mem.hh"

namespace game {
// This block allocator is suitable for allocating mostly static memory.
// This block allocator can allocate up to block size (default is 64 KiB) in a single allocation. Larger single allocations will fail.
// This block allocator does not free memory. You must destroy the block allocator to free all the memory.
struct MemBlockAllocator {
  byte* first_block_;
  byte* last_block_;
  i32   unused_bytes_in_block_;
  i32   block_size_;

  void Create(i32 block_size = 64 * 1024) {
    MemZeroInit(this);
    block_size_ = block_size;
  }

  void Destroy() {
    auto block = first_block_;
    for (; block != nullptr;) {
      auto next = ((byte**)block)[0];
      MemFree(MEM_ALLOC_HEAP, block);
      block = next;
    }
    first_block_           = nullptr;
    last_block_            = nullptr;
    unused_bytes_in_block_ = 0;
    block_size_            = 0;
  }

  // ---

  void* _Allocate(i32 size, i32 alignment) {
    assert(size <= (block_size_ - MEM_CACHE_LINE_SIZE) && "requested memory must be less than allocator block size");

    if (!((MemAlign(block_size_ - unused_bytes_in_block_, alignment) + size) <= block_size_)) {
      auto new_block         = (byte*)MemAlloc(MEM_ALLOC_HEAP, block_size_, MEM_CACHE_LINE_SIZE);
      ((byte**)new_block)[0] = nullptr;
      if (first_block_ == nullptr) {
        first_block_ = new_block;
      }
      if (!(last_block_ == nullptr)) {
        ((byte**)last_block_)[0] = new_block;
      }
      last_block_            = new_block;
      unused_bytes_in_block_ = block_size_ - 8;
    }

    auto offset = MemAlign(block_size_ - unused_bytes_in_block_, alignment);

    auto ptr               = last_block_ + offset;
    unused_bytes_in_block_ = (block_size_ - (offset + size));
    return ptr;
  }

  // Allocate uninitialized memory for object of size T (requested memory must be less than allocator block size)
  template <typename T> T* Allocate() {
    return (T*)_Allocate((i32)sizeof(T), (i32)alignof(T));
  }

  // Allocate uninitialized memory for array of size T * n (requested memory must be less than allocator block size)
  template <typename T> T* AllocateArray(i32 n) {
    return (T*)_Allocate((i32)sizeof(T) * n, (i32)alignof(T));
  }

  // Allocate uninitialized memory for slice of size T * n (requested memory must be less than allocator block size)
  template <typename T> Slice<T> AllocateSlice(i32 cap) {
    return Slice<T>{ AllocateArray<T>(cap), 0, cap };
  }
};
} // namespace game