#include "mem.hh"

#include "../test/test.h"

using namespace game;

struct alignas(2) AlignAs2 {
  char a_[2];
};
struct alignas(4) AlignAs4 {
  char a_[4];
};
struct alignas(8) AlignAs8 {
  char a_[8];
};

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  // This test will fail if there's a memory leak
  // TEST_CASE("MemAllocLeak") {
  //   MemAlloc(MEM_ALLOC_HEAP, 8, 8);
  // }

  TEST_CASE("MemAlloc") {
    byte* ptr = (byte*)MemAlloc(MEM_ALLOC_HEAP, 64, 8);
    ASSERT_TRUE(ptr);
    ASSERT_EQUAL_U64(0, (u64)ptr & 7);
    for (byte i = 0; i < 64; i++) {
      ptr[i] = i;
    }
    MemFree(MEM_ALLOC_HEAP, ptr);
  }

  TEST_CASE("MemAlloc (alignas)") {
    AlignAs2* align_as_2 = MemAlloc<AlignAs2>(MEM_ALLOC_HEAP);
    ASSERT_TRUE(align_as_2);
    ASSERT_EQUAL_U64(0, (u64)align_as_2 & 1);

    AlignAs4* align_as_4 = MemAlloc<AlignAs4>(MEM_ALLOC_HEAP);
    ASSERT_TRUE(align_as_4);
    ASSERT_EQUAL_U64(0, (u64)align_as_4 & 3);

    AlignAs8* align_as_8 = MemAlloc<AlignAs8>(MEM_ALLOC_HEAP);
    ASSERT_TRUE(align_as_8);
    ASSERT_EQUAL_U64(0, (u64)align_as_8 & 7);

    MemFree(MEM_ALLOC_HEAP, align_as_2);
    MemFree(MEM_ALLOC_HEAP, align_as_4);
    MemFree(MEM_ALLOC_HEAP, align_as_8);
  }

  return 0;
}