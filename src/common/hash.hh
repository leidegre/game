#pragma once

#define XXH_INLINE_ALL
#include <xxhash.h>

#include "type-system.hh"

namespace game {
inline u32 HashData(const void* data, i32 size, u32 seed = 0) {
  return u32(XXH32(data, size_t(size), XXH32_hash_t(seed)));
}

template <typename T> u32 HashData(const Slice<T>& data, u32 seed = 0) {
  return HashData(data.ptr_, data.ByteLen(), seed);
}

// 32-bit hash code builder
struct Hash32 {
  XXH32_state_t state_;

  // Initialize (or reset) hash.
  void Init(u32 seed = 0) {
    XXH32_reset(&state_, seed);
  }

  // Hash some data (you must call Init at least once before calling Update).
  void Update(const void* data, i32 size) {
    XXH32_update(&state_, data, size_t(size));
  }

  // Hash some data (you must call Init at least once before calling Update).
  template <typename T> void Update(const Slice<T>& data) {
    XXH32_update(&state_, data.ptr_, size_t(data.ByteLen()));
  }

  // Extract the final hash code
  u32 Digest() const {
    return XXH32_digest(&state_);
  }
};
} // namespace game
