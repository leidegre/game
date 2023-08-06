#include "../test/test.h"

#include "hash-map.hh"

using namespace game;

namespace {
i32 HashInt(i32 v) {
  return HashData(&v, 4);
}
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("HashMapTest") {
    HashMap<int> map;

    map.Create(MEM_ALLOC_HEAP, 16);

    int i = 1;

    for (; i <= 11; i++) {
      map.Add(HashInt(i), i);
    }

    ASSERT_EQUAL_I32(16, map.Cap());

    for (auto entry : map.Scan(HashInt(11))) {
      ASSERT_EQUAL_I32(11, entry.Value());
    }

    for (auto entry : map.Scan(HashInt(12))) {
      ASSERT_TRUE(false);
    }

    for (; i <= 22; i++) {
      map.Add(HashInt(i), i);
    }

    ASSERT_EQUAL_I32(32, map.Cap());

    for (auto entry : map.Scan(HashInt(12))) {
      ASSERT_EQUAL_I32(12, entry.Value());
    }

    for (auto entry : map.Scan(HashInt(13))) {
      ASSERT_EQUAL_I32(13, entry.Value());
    }

    for (auto entry : map.Scan(HashInt(14))) {
      ASSERT_EQUAL_I32(14, entry.Value());
    }

    for (auto entry : map.Scan(HashInt(23))) {
      ASSERT_TRUE(false);
    }

    map.Destroy();
  }
}
