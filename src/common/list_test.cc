#include "../test/test.h"

#include "list.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("ListTest") {
    List<int> list = { nullptr, 0, 0, MEM_ALLOC_HEAP };

    list.Add(1);
    list.Add(2);

    int i = 1;
    for (auto value : list) {
      ASSERT_EQUAL_I32(i++, value);
    }

    list.RemoveAtSwapBack(0);

    for (auto value : list) {
      ASSERT_EQUAL_I32(2, value);
    }

    list.Destroy();
  }
}
