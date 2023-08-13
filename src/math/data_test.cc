#include "math.hh"

#include "../test/test.h"

using namespace game;
using namespace math;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("mat3 init (column-major)") {
    mat3 m = {
      {1, 2, 3},
      {4, 5, 6},
      {7, 8, 9},
    };

    ASSERT_EQUAL_FLOAT(1, m.c0.x, 0);
    ASSERT_EQUAL_FLOAT(2, m.c0.y, 0);
    ASSERT_EQUAL_FLOAT(3, m.c0.z, 0);

    ASSERT_EQUAL_FLOAT(4, m.c1.x, 0);
    ASSERT_EQUAL_FLOAT(5, m.c1.y, 0);
    ASSERT_EQUAL_FLOAT(6, m.c1.z, 0);

    ASSERT_EQUAL_FLOAT(7, m.c2.x, 0);
    ASSERT_EQUAL_FLOAT(8, m.c2.y, 0);
    ASSERT_EQUAL_FLOAT(9, m.c2.z, 0);
  }
}
