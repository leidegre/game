#include "../test/test.h"

#include "hash.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("HashTest") {
    byte temp[16];

    for (byte i = 0; i < 16; i++) {
      temp[i] = i;
    }

    ASSERT_EQUAL_U32(46947589, HashData(temp, 0));
    ASSERT_EQUAL_U32(3479547966, HashData(temp, 1));
    ASSERT_EQUAL_U32(3329353656, HashData(temp, 2));
    ASSERT_EQUAL_U32(1715378773, HashData(temp, 3));
    ASSERT_EQUAL_U32(2154372710, HashData(temp, 4));
    ASSERT_EQUAL_U32(3072866292, HashData(temp, 16));
  }
}
