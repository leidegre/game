#pragma once

#include "hash.hh"
#include "mem.hh"

namespace game {
template <typename T> struct HashMapScan;

template <typename T> struct HashMapIterator;

// Must be initialized before use. Call Create() with an initial capacity to initialize hash map.
// It does not resolve hash collisions nor does it compute hashes.
// This way the HashMap is not tied to a particular hash function and it can be used as a multi-map when needed.
template <typename T> struct HashMap {
  enum HashCode : u32 {
    ZERO_CODE = 0x0,
    SKIP_CODE = 0xFFFFFFFF,
    SAFE_CODE = 0x1, // it doesn't matter but it cannot be zero or skip
  };

  enum { MIN_CAPACITY = MEM_CACHE_LINE_SIZE / 4 };

  static u32 SafeHash(u32 hash) {
    if ((hash == ZERO_CODE) | (hash == SKIP_CODE)) {
      hash = SAFE_CODE;
    }
    return hash;
  }

  u32*      hashes_;
  T*        values_;
  u32       cap_;
  i32       zero_node_count_;
  i32       skip_node_count_;
  Allocator allocator_;

  // Unused space in hash map
  i32 Available() {
    return zero_node_count_ + skip_node_count_;
  }

  // Actual number of items in map
  i32 Len() {
    return Cap() - Available();
  }

  i32 Cap() {
    return i32(cap_);
  }

  // initial_capacity must be a power of 2
  void Create(Allocator allocator, i32 initial_capacity) {
    if (initial_capacity < MIN_CAPACITY) {
      initial_capacity = MIN_CAPACITY;
    }

    hashes_          = MemAllocArray<u32>(allocator, initial_capacity);
    values_          = MemAllocArray<T>(allocator, initial_capacity);
    cap_             = u32(initial_capacity);
    zero_node_count_ = initial_capacity;
    skip_node_count_ = 0;
    allocator_       = allocator;

    memset(hashes_, ZERO_CODE, initial_capacity * 4);
  }

  void Destroy() {
    MemFree(allocator_, hashes_);
    MemFree(allocator_, values_);

    hashes_          = nullptr;
    values_          = nullptr;
    cap_             = 0;
    zero_node_count_ = 0;
    skip_node_count_ = 0;
    allocator_       = MEM_ALLOC_NONE;
  }

  // Get the address of value at index.
  T* _AddrOf(i32 index) {
    return values_ + index;
  }

  // Get the reference of value at index.
  T& operator[](i32 index) {
    return values_[index];
  }

  void Swap(HashMap<T>* other) {
    auto hashes          = other->hashes_;
    auto values          = other->values_;
    auto cap             = other->cap_;
    auto zero_node_count = other->zero_node_count_;
    auto skip_node_count = other->skip_node_count_;
    auto allocator       = other->allocator_;

    other->hashes_          = this->hashes_;
    other->values_          = this->values_;
    other->cap_             = this->cap_;
    other->zero_node_count_ = this->zero_node_count_;
    other->skip_node_count_ = this->skip_node_count_;
    other->allocator_       = this->allocator_;

    this->hashes_          = hashes;
    this->values_          = values;
    this->cap_             = cap;
    this->zero_node_count_ = zero_node_count;
    this->skip_node_count_ = skip_node_count;
    this->allocator_       = allocator;
  }

  void Clear() {
    memset(hashes_, ZERO_CODE, 4 * cap_);
    zero_node_count_ = cap_;
    skip_node_count_ = 0;
  }

  // Scan hash map for hash collisions
  HashMapScan<T> Scan(u32 key_hash);

  // Unconditionally add value with hash.
  bool Add(u32 key_hash, const T& value) {
    auto safe_hash = SafeHash(key_hash);
    for (u32 n = 0; n != cap_; ++n) {
      auto i    = (safe_hash + n) & (cap_ - 1);
      auto hash = hashes_[i];
      if (hash == ZERO_CODE) {
        hashes_[i] = safe_hash;
        memcpy(_AddrOf(i), &value, sizeof(T));
        zero_node_count_--;
        _MaybeGrow();
        return true;
      }
      if (hash == SKIP_CODE) {
        hashes_[i] = safe_hash;
        memcpy(_AddrOf(i), &value, sizeof(T));
        skip_node_count_--;
        _MaybeGrow();
        return true;
      }
    }
    // Add will only fail if the hash map was not initialized
    assert(false && "cannot insert into HashMap. did you forget to initialize HashMap?");
    return false;
  }

  // new_capacity must be a power of 2
  void Resize(i32 new_capacity) {
    if (new_capacity < MIN_CAPACITY) {
      new_capacity = MIN_CAPACITY;
    }
    if (new_capacity == Cap()) {
      return;
    }
    HashMap<T> temp;
    temp.Create(allocator_, new_capacity);
    for (int i = 0; i < Cap(); i++) {
      auto hash = hashes_[i];
      if ((hash != ZERO_CODE) & (hash != SKIP_CODE)) {
        temp.Add(hash, this->operator[](i));
      }
    }
    temp.Swap(this);
    temp.Destroy();
  }

  void _MaybeGrow() {
    if (Available() < Cap() / 3) {
      Resize(2 * Cap());
    }
  }

  void _MaybeShrink() {
    if (Len() < Cap() / 3) {
      Resize(Cap() / 2);
    }
  }

  void RemoveAt(i32 index) {
    hashes_[index] = SKIP_CODE;
    skip_node_count_++;
    _MaybeShrink();
  }

  HashMapIterator<T> begin();
  HashMapIterator<T> end();
};

template <typename T> struct HashMapScanIterator {
  HashMap<T>* map_;
  u32         hash_;
  u32         n_;

  bool IsValid() {
    return n_ != map_->cap_;
  }

  i32 Index() {
    return (i32)((hash_ + n_) & (map_->cap_ - 1));
  }

  T Value() {
    return map_->operator[](Index());
  }

  // Update value in place
  void Update(const T& value) {
    map_->operator[](Index()) = value;
  }

  void Remove() {
    map_->RemoveAt(Index());
  }

  HashMapScanIterator& operator++() {
    ++n_;
    for (; IsValid(); ++n_) {
      auto hash = map_->hashes_[Index()];
      if (hash == hash_) {
        return *this; // ok
      }
      if (hash == HashMap<T>::ZERO_CODE) {
        n_ = (u32)map_->Cap();
        break;
      }
    }
    return *this;
  }

  bool operator!=(const HashMapScanIterator& other) {
    return this->n_ != other.n_;
  }

  HashMapScanIterator& operator*() {
    return *this;
  }
};

template <typename T> struct HashMapScan {
  HashMap<T>* map_;
  u32         hash_; // Cannot be zero or skip code

  HashMapScanIterator<T> begin() {
    return ++HashMapScanIterator<T>{ map_, hash_, 0xFFFFFFFFU };
  }

  HashMapScanIterator<T> end() {
    return HashMapScanIterator<T>{ map_, hash_, map_->cap_ };
  }
};

template <typename T> HashMapScan<T> HashMap<T>::Scan(u32 key_hash) {
  return HashMapScan<T>{ this, SafeHash(key_hash) };
}

// ---

template <typename T> struct HashMapIterator {
  HashMap<T>* map_;
  u32         n_;

  HashMapIterator<T>& operator++() {
    ++n_;
    for (; n_ != map_->cap_; ++n_) {
      auto hash = map_->hashes_[n_];
      if ((hash == ZERO_CODE) | (hash == SKIP_CODE)) {
        continue;
      }
      break;
    }
    return *this;
  }

  bool operator!=(const HashMapIterator<T>& other) {
    return this->n_ != other.n_;
  }

  T& operator*() {
    return map_->operator[](i32(n_));
  }
};

template <typename T> HashMapIterator<T> HashMap<T>::begin() {
  return ++HashMapIterator<T>{ this, 0xFFFFFFFF };
}

template <typename T> HashMapIterator<T> HashMap<T>::end() {
  return HashMapIterator<T>{ this, cap_ };
}
} // namespace game