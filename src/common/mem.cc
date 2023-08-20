#include "mem.hh"

#include <cstdlib>

using namespace game;

// for now having this here is fine but we may need to grow it and introduce a MemInit/MemShutdown
static byte s_temp[1024 * 1024];
static i32  s_temp_begin;

byte* game::MemAlloc(Allocator allocator, size_t size, size_t alignment) {
  switch (allocator) {
  case MEM_ALLOC_NONE:
    assert(false && "MEM_ALLOC_NONE cannot be used with MemAlloc");
    abort();
    return nullptr;

  case MEM_ALLOC_HEAP: {
    byte* block;
// The CRT debug heap does not support aligned malloc with type information
// i.e. there's no way for us to allocate the _CLIENT_BLOCK via _aligned_malloc
#if _WIN32
    block = (byte*)_aligned_malloc(size, alignment);
#elif __clang__
    block = (byte*)aligned_alloc(alignment, MemAlign(size, alignment)); // swapped...
#else
#error Don't know how to allocate aligned memory
#endif
    assert(block && "cannot allocate memory from heap");
    return block;
  }

  case MEM_ALLOC_TEMP: {
    i32 aligned = MemAlign(s_temp_begin, alignment);
    assert(aligned + size <= sizeof(s_temp) && "MEM_ALLOC_TEMP: out of memory");
    byte* ptr    = &s_temp[aligned];
    s_temp_begin = aligned + size;
    return ptr;
  }

  default:
    assert(false && "unknown allocator"); // checked or not...
    abort();
    return nullptr;
  }
}

void game::MemResetTemp() {
  s_temp_begin = 0;
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

  case MEM_ALLOC_TEMP: {
    break; // freeing temp memory does nothing, use MemResetTemp to reset temporary memory
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
