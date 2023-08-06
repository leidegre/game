#pragma once

#include <cassert>
#include <cstdint>

namespace game {
using byte = uint8_t;

using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;

template <i32 N, typename T> constexpr i32 ArrayLength(T (&array)[N]) {
  return N;
}

// A slice is a view of memory that may or may not be read only.
// If slice is passed as Slice<const T> the memory is read only.
template <typename T> struct Slice {
  T*  ptr_;
  i32 len_;
  i32 cap_;

  // Length of slice in number of elements.
  i32 Len() const {
    return len_;
  }

  // Size of slice in bytes (the total size of number of elements in bytes).
  i32 ByteLen() const {
    return (i32)sizeof(T) * len_;
  }

  i32 Cap() const {
    return cap_;
  }

  T& operator[](i32 index) const {
    assert((0 <= index) & (index < len_));
    return ptr_[index];
  }

  // Make writable slice into a read-only slice
  Slice<const T> Const() const {
    return { ptr_, len_, cap_ };
  }

  // ranged based for loop support

  T* begin() const {
    return ptr_;
  }

  T* end() const {
    return ptr_ + len_;
  }
};
} // namespace game
