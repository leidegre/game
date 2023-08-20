#include "math.hh"

#include "../test/test.h"

#include <cstring>

using namespace game;
using namespace math;

namespace {
mat4 MulNaive(const mat4& a, const mat4& b) {
  // https://en.wikipedia.org/wiki/Matrix_multiplication#Definition
  mat4 tmp;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      f32 sum = 0;
      for (int k = 0; k < 4; k++) {
        sum += b.Column()[j].Array()[k] * a.Column()[k].Array()[i];
      }
      tmp.Column()[j].Array()[i] = sum;
    }
  }
  return tmp;
}
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("mat4 mul (naive)") {
    mat4 a = mat4::Identity();
    mat4 b = mat4::Identity();

    mat4 ab = MulNaive(a, b);

    ASSERT_TRUE(memcmp(&mat4::Identity(), &ab, 64) == 0);
  }

  TEST_CASE("mat4 mul (naive 1,2,3...)") {
    // https://www.symbolab.com/solver/matrix-multiply-calculator/%5Cbegin%7Bpmatrix%7D1%262%263%264%5C%5C%20%20%205%266%267%268%5C%5C%20%20%209%2610%2611%2612%5C%5C%20%20%2013%2614%2615%2616%5Cend%7Bpmatrix%7D%5Cbegin%7Bpmatrix%7D17%2618%2619%2620%5C%5C%20%20%2021%2622%2623%2624%5C%5C%20%20%2025%2626%2627%2628%5C%5C%20%20%2029%2630%2631%2632%5Cend%7Bpmatrix%7D?or=input

    mat4 a = {
      {1, 5,  9, 13},
      {2, 6, 10, 14},
      {3, 7, 11, 15},
      {4, 8, 12, 16},
    };

    mat4 b = {
      {17, 21, 25, 29},
      {18, 22, 26, 30},
      {19, 23, 27, 31},
      {20, 24, 28, 32},
    };

    mat4 expected = {
      {250, 618,  986, 1354},
      {260, 644, 1028, 1412},
      {270, 670, 1070, 1470},
      {280, 696, 1112, 1528},
    };

    mat4 actual = MulNaive(a, b);

    ASSERT_TRUE(memcmp(&expected, &actual, 64) == 0);
  }

  TEST_CASE("mat4 mul") {
    mat4 expected = {
      { 1585,  1655,  1787,  1861},
      { 5318,  5562,  5980,  6246},
      {10514, 11006, 11840, 12378},
      {15894, 16634, 17888, 18710},
    };
    Transpose(&expected); // row major initialization

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

    mat4 actual_naive = MulNaive(a, b);
    mat4 actual       = Mul(a, b);

    // We're using small prime numbers and there's no precision loss
    // therefor these matrices should be bitwise identical

    ASSERT_TRUE(memcmp(&expected, &actual_naive, 64) == 0);
    ASSERT_TRUE(memcmp(&expected, &actual, 64) == 0);
  }
}