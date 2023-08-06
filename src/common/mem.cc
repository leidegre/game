#include "mem.hh"

#include <cstdlib>

using namespace game;

void* game::MemAlloc(Allocator allocator, i32 size, i32 alignment) {
  switch (allocator) {
  case MEM_ALLOC_NONE:
    assert(false && "unsupported allocator");
    return nullptr;
  case MEM_ALLOC_HEAP: {
// The CRT debug heap does not support aligned malloc with type information
// i.e. there's no way for us to allocate the _CLIENT_BLOCK via _aligned_malloc
#if _WIN32
    auto block = _aligned_malloc(size_t(size), size_t(alignment));
#elif __clang__
    auto block = aligned_alloc(size_t(alignment), size_t(MemAlign(size, alignment))); // swapped...
#else
#error Don't know how to allocate aligned memory
#endif
    assert(block && "cannot allocate memory from heap");
    return block;
  }
  default:
    assert(false && "unknown allocator"); // checked or not...
    abort();
    return nullptr;
  }
}

void game::MemFree(Allocator allocator, void* block) {
  switch (allocator) {
  case MEM_ALLOC_NONE: {
    break;
  }
  case MEM_ALLOC_HEAP: {
#if _WIN32
    _aligned_free(block);
#elif __clang__
    free(block);
#else
#error Don't know how to deallocate aligned memory
#endif
    break;
  }
  default: {
    assert(false && "unknown allocator");
    abort();
    break;
  }
  }
}

bool game::MemIsZero(const void* block, i32 size) {
  auto data32 = (const u32*)block;

  i32 i = 0;
  u32 z;

  for (; i + 15 < size; i += 16) {
    u32 a = data32[0];
    u32 b = data32[1];
    u32 c = data32[2];
    u32 d = data32[3];

    data32 += 4;

    z = a | b | c | d;
    if (z != 0) {
      return false;
    }
  }

  auto data8 = (const byte*)data32;

  z = 0;
  for (; i < size; i++) {
    z |= *data8++;
  }
  if (z != 0) {
    return false;
  }

  return true;
}
