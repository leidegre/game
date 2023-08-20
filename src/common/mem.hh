#pragma once

#include "type-system.hh"

#include <malloc.h>

#if _WIN32
#else
#include <alloca.h>
#endif

#include <cstring>

#include <initializer_list> // ???

// Can be used to temporarily allocate a small amount of memory for a slice on stack (~kilobytes). Like a variable length arrays (VLA).
// The memory will be zero initialized.
// The memory will be invalid as soon as the function using the stackalloc macro returns.
// Do not use within exception handling.
#define MemStackalloc(T, len, cap)                                                                                     \
  (::game::Slice<T>{ (T*)memset(alloca(size_t(cap) * sizeof(T)), 0, size_t(cap) * sizeof(T)), i32(len), i32(cap) })

namespace game {
// Test the contents of this slice against some other slice for bitwise equality (memcmp)
template <typename T> bool Equals(const Slice<T>& a, const Slice<T>& b) {
  return a.len_ == b.len_ && memcmp(a.ptr_, b.ptr_, size_t(a.len_)) == 0;
}

// Append value to end of slice. Slice can be empty but must have preallocated spare capacity.
template <typename T> Slice<T> Append(Slice<T> slice, T value) {
  assert(slice.Len() + 1 <= slice.Cap());
  slice.ptr_[slice.len_] = value;
  return Slice<T>{ slice.ptr_, slice.len_ + 1, slice.cap_ };
};

// Append other slice to end of slice (memcmp). Left most slice can be empty but must have preallocated spare capacity.
template <typename T> Slice<T> Append(Slice<T> slice, const Slice<T>& other) {
  assert(slice.Len() + other.Len() <= slice.Cap());
  memcpy(slice.end(), other.begin(), other.ByteLen());
  return Slice<T>{ slice.ptr_, slice.len_ + other.Len(), slice.cap_ };
};

// Insert value in into sorted slice. Slice can be empty but must have allocated spare capacity.
template <typename T> Slice<T> Insert(Slice<T> sorted, T value) {
  assert(sorted.Len() + 1 <= sorted.Cap());
  int len = sorted.len_;
  for (; len > 0 && value < sorted[len - 1]; len--) {
    sorted.ptr_[len] = sorted[len - 1];
  }
  sorted.ptr_[len] = value;
  return Slice<T>{ sorted.ptr_, sorted.len_ + 1, sorted.cap_ };
};

namespace slice {
// Create read only slice from initializer_list
// Note that this does not extend the lifetime of the initializer_list it is just a wrapper for the initializer_list interface; use responsibly
// WARNING: This is not legal slice::FromInitializer({1, 2, 3})
template <typename T> constexpr Slice<const T> FromInitializer(std::initializer_list<T> initializer_list) {
  i32 length = (i32)initializer_list.size();
  return { initializer_list.begin(), length, length };
};
} // namespace slice

enum Allocator {
  // Memory came from somewhere else. Cannot realloc or free this memory.
  MEM_ALLOC_NONE = 0,

  // Memory allocated from heap. You must eventually free this memory.
  MEM_ALLOC_HEAP,

  // Temp memory. Temp memory is very short lived. Think of it as memory allocated on the stack. You can free temp memory but it doesn't leak if you don't.
  MEM_ALLOC_TEMP,

  // Job memory. Job memory has longer duration than temp memory but it is not meant to stick around for more than a few frames.
  MEM_ALLOC_TEMP_JOB,
};

enum {
  // When the CPU fetches from memory it doesn't fetch a byte or a word it fetches a cache line. In a multi-core system, if a cache line is shared, a cache coherency protocol must be used to maintain a coherent view of the data. Cache coherency protocols can kill performance (see false sharing) and sometimes, by taking up a whole cache line, we can ensure that data is not shared by accident.
  MEM_CACHE_LINE_SIZE = 64,
  MEM_CACHE_LINE_BITS = 6, // (1 << MEM_CACHE_LINE_BITS) == MEM_CACHE_LINE_SIZE
};

// Returns the smallest power of two greater than or equal to the input.
inline i32 MemCeilPow2(i32 x) {
  x -= 1;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

inline size_t MemAlign(size_t size, size_t alignment) {
  return (size + (alignment - 1)) & ~(alignment - 1);
}

// todo: return byte* instead of void* more useful cast is required anyway
// todo: maybe use size_t all the way... i32 is a bit clunky and overloads is a bit annoying

// MemAlloc[Array][ZeroInit]

// Allocate uninitialized memory
byte* MemAlloc(Allocator allocator, size_t size, size_t alignment);

// Allocate uninitialized memory
template <typename T> T* MemAlloc(Allocator allocator, size_t size = sizeof(T), size_t alignment = alignof(T)) {
  return (T*)MemAlloc(allocator, size, alignment);
}

// Allocate uninitialized array
inline void* MemAllocArray(Allocator allocator, i32 count, i32 size, i32 alignment) {
  return MemAlloc(allocator, count * size, alignment);
}

// Allocate uninitialized array
template <typename T>
T* MemAllocArray(Allocator allocator, i32 count, i32 size = i32(sizeof(T)), i32 alignment = i32(alignof(T))) {
  return (T*)MemAlloc(allocator, count * size, alignment);
}

// Allocate uninitialized slice
template <typename T> inline Slice<T> MemSlice(Allocator allocator, i32 count, i32 capacity) {
  return Slice<T>{ (T*)MemAllocArray(allocator, capacity, i32(sizeof(T)), i32(alignof(T))), 0, capacity };
}

// Allocate zero initialized memory
inline byte* MemAllocZeroInit(Allocator allocator, i32 size, i32 alignment) {
  byte* ptr = MemAlloc(allocator, size, alignment);
  memset(ptr, 0, size_t(size));
  return ptr;
}

// Allocate zero initialized memory
template <typename T> T* MemAllocZeroInit(Allocator allocator) {
  auto block = MemAlloc<T>(allocator);
  memset(block, 0, sizeof(T)); // will generate warning if T is virtual
  return (T*)block;
}

// Allocate zero initialized array of dynamic length
template <typename T> T* MemAllocZeroInitArray(Allocator allocator, i32 n) {
  auto block = MemAlloc(allocator, n * i32(sizeof(T)), i32(alignof(T)));
  memset(block, 0, size_t(n) * sizeof(T));
  return (T*)block;
}

// Reset temporary memory allocations.
void MemResetTemp();

// Copy some number of bytes (size) from src to dst.
inline void* MemCopy(void* dst, const void* src, i32 size) {
  return memcpy(dst, src, size_t(size));
}

// (memcpy) Copy some number of array elements from src to dst. Note that len is count in number of elements not size in bytes.
template <typename T> T* MemCopyArray(T* dst, const T* src, i32 len) {
  return (T*)memcpy(dst, src, i32(sizeof(T)) * len);
}

void MemFree(Allocator allocator, void* block);

// Zero initialize
template <typename T> void MemZeroInit(T* block) {
  memset(block, 0, sizeof(T));
}

// Zero initialize
inline void MemZeroInit(void* block, i32 size) {
  memset(block, 0, size_t(size));
}

// Test if block is zero initialized
bool MemIsZero(const void* block, i32 size);

template <typename T> T* MemResizeArray(Allocator allocator, T* old_ptr, i32 old_len, i32 new_len) {
  T* new_ptr = nullptr;
  if (0 < new_len) {
    new_ptr = MemAllocArray<T>(allocator, new_len);
    if (0 < old_len) {
      memcpy(new_ptr, old_ptr, i32(sizeof(T)) * old_len);
    }
  }
  if (0 < old_len) {
    MemFree(allocator, old_ptr);
  }
  return new_ptr;
}
} // namespace game

// ---

namespace game {
inline i32 Min(i32 x, i32 y) {
  return x < y ? x : y;
}

inline i32 Min(i32 x, i32 y, i32 z) {
  return Min(Min(x, y), z);
}

inline i32 Max(i32 x, i32 y) {
  return x < y ? y : x;
}

inline i32 Max(i32 x, i32 y, i32 z) {
  return Max(Max(x, y), z);
}

inline i64 Min64(i64 x, i64 y) {
  return x < y ? x : y;
}

inline u64 Max64(u64 x, u64 y) {
  return x < y ? y : x;
}
} // namespace game
