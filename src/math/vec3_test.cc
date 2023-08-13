#include "math.hh"

#include "../test/test.h"

using namespace game;
using namespace math;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("vec3 cross") {
    vec3 a = { 1, 0, 0 };
    vec3 b = { 0, 1, 0 };
    vec3 c = Cross(a, b);
    ASSERT_EQUAL_FLOAT(1, c.z, 0);
  }

  TEST_CASE("vec3 add") {
    vec3 a = { 2, 3, 5 };
    vec3 b = { 7, 11, 13 };
    vec3 c = (a + b);
    ASSERT_EQUAL_FLOAT(9, c.x, 0);
    ASSERT_EQUAL_FLOAT(14, c.y, 0);
    ASSERT_EQUAL_FLOAT(18, c.z, 0);
  }
}
