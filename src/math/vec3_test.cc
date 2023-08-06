#include "vec3.hh"

#include "../test/test.h"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("vec3 cross") {
    vec3 a = { 1, 0, 0 };
    vec3 b = { 0, 1, 0 };
    vec3 c = Cross(a, b);
    ASSERT_EQUAL_FLOAT(1, c.v_[2], 0);
  }

  TEST_CASE("vec3 add") {
    vec3 a = { 2, 3, 5 };
    vec3 b = { 7, 11, 13 };
    vec3 c = (a + b);
    ASSERT_EQUAL_FLOAT(9, c.v_[0], 0);
    ASSERT_EQUAL_FLOAT(14, c.v_[1], 0);
    ASSERT_EQUAL_FLOAT(18, c.v_[2], 0);
  }
}
