#pragma once

#include "mem.hh"

namespace game {
// Dynamic array list (can grow as needed to accommodate elements as long as it has a useable allocator)
template <typename T> struct List {
  // Create a zero initialized list that will invoke the allocator when the first element is added to the list.
  static List<T> WithAllocator(Allocator allocator) {
    return { nullptr, 0, 0, allocator };
  }

  // This will create a fixed size list around the memory provided. The list cannot grow/shrink dynamically if created this way.
  static List<T> FromArray(T* arr, i32 len) {
    return { arr, len, len, MEM_ALLOC_NONE };
  }

  enum { MIN_CAPACITY = MEM_CACHE_LINE_SIZE / sizeof(T) };

  T*        ptr_;
  i32       len_;
  i32       cap_;
  Allocator allocator_;

  T& operator[](i32 index) {
    assert(index < len_);
    return ptr_[index];
  }

  // Length of list in number of elements
  i32 Len() {
    return len_;
  }

  // Size of list in number of bytes
  i32 ByteLen() {
    return len_ * i32(sizeof(T));
  }

  i32 Cap() {
    return cap_;
  }

  // Append element to list
  void Add(const T& value) {
    if (nullptr == ptr_) {
      SetCapacity(MIN_CAPACITY);
    } else if (!(len_ + 1 <= cap_)) {
      SetCapacity(2 * cap_);
    }
    ptr_[len_++] = value;
  }

  void AddRange(const List<T>& other) {
    if (nullptr == ptr_) {
      SetCapacity(other.len_);
    } else if (!(len_ + other.len_ <= cap_)) {
      SetCapacity(len_ + other.len_);
    }
    memcpy(ptr_, other.ptr_, size_t(ByteLen()));
  }

  void RemoveAtSwapBack(i32 index) {
    ptr_[index] = ptr_[len_ - 1];
    len_--;
  }

  // Remove last element in list (as if the list was a LIFO stack)
  void Pop() {
    assert(0 < len_);
    len_--;
  }

  // Get last element in list (as if the list was a LIFO stack)
  T Peek() {
    assert(0 < len_);
    return ptr_[len_ - 1];
  }

  void Create(Allocator allocator, i32 initial_capacity) {
    ptr_       = nullptr;
    len_       = 0;
    cap_       = 0;
    allocator_ = allocator;

    SetCapacity(initial_capacity);
  }

  void Destroy() {
    MemFree(allocator_, ptr_);
    ptr_ = nullptr;
    len_ = 0;
    cap_ = 0;
  }

  // Sets the capacity of the list. The length remains unchanged unless the capacity is made to be less than the length of the list.
  void SetCapacity(i32 new_capacity) {
    if (new_capacity < MIN_CAPACITY) {
      new_capacity = MIN_CAPACITY;
    }
    if (new_capacity == cap_) {
      return;
    }
    ptr_ = MemResizeArray(allocator_, ptr_, len_, new_capacity);
    len_ = Min(len_, new_capacity);
    cap_ = new_capacity;
  }

  void Resize(i32 new_capacity) {
    if (cap_ < new_capacity) {
      SetCapacity(new_capacity);
    }
    len_ = new_capacity;
  }

  // Zero fill all allocated memory
  void Clear() {
    memset(ptr_, 0, cap_ * sizeof(T));
  }

  void Swap(List<T>* other) {
    auto ptr = this->ptr_;
    auto len = this->len_;
    auto cap = this->cap_;

    this->ptr_ = other->ptr_;
    this->len_ = other->len_;
    this->cap_ = other->cap_;

    other->ptr_ = ptr;
    other->len_ = len;
    other->cap_ = cap;
  }

  // iterator protocol, adds support for range-based for loops

  T* begin() {
    return ptr_;
  }

  T* end() {
    return ptr_ + len_;
  }
};
} // namespace game