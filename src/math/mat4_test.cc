#include "math.hh"

#include "../test/test.h"

#include <cstring>

using namespace game;
using namespace math;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("mat4 mul") {
    mat4 a = {
      { 2,  3,  5,  7},
      {11, 13, 17, 19},
      {23, 29, 31, 37},
      {41, 43, 47, 53}
    };
    Transpose(&a); // row major initialization

    mat4 b = {
      { 59,  61,  67,  71},
      { 73,  79,  83,  89},
      { 97, 101, 103, 107},
      {109, 113, 127, 131}
    };
    Transpose(&b); // row major initialization

    mat4 actual = Mul(a, b);

    mat4 expected = {
      { 1585,  1655,  1787,  1861},
      { 5318,  5562,  5980,  6246},
      {10514, 11006, 11840, 12378},
      {15894, 16634, 17888, 18710},
    };
    Transpose(&expected); // row major initialization

    // We're using small prime numbers and
    // they don't suffer from precision loss
    // the matrices should be exactly equal

    ASSERT_TRUE(memcmp(&actual, &expected, 64) == 0);
  }
}