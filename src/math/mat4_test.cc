#include "mat4.hh"

#include "../test/test.h"

#include <cstdio>
#include <cstring>

using namespace game;

namespace {
// bitwise equals
template <typename T> bool Equals(const T& a, const T& b) {
  return memcmp(&a, &b, sizeof(T)) == 0;
}

// like equals but we tolerate small differences (EPSILON)
bool AlmostEquals(const vec4& a, const vec4& b) {
  vec4 tmp = {
    a.v_[0] - b.v_[0],
    a.v_[1] - b.v_[1],
    a.v_[2] - b.v_[2],
    a.v_[3] - b.v_[3],
  };

  return (fabs(tmp.v_[0]) < EPSILON) & // x
         (fabs(tmp.v_[1]) < EPSILON) & // y
         (fabs(tmp.v_[2]) < EPSILON) & // z
         (fabs(tmp.v_[3]) < EPSILON);
}

bool IsIdentity(const mat4& m) {
  return AlmostEquals(m.Row(0), { 1, 0, 0, 0 }) & // row 0
         AlmostEquals(m.Row(1), { 0, 1, 0, 0 }) & // row 1
         AlmostEquals(m.Row(2), { 0, 0, 1, 0 }) & // row 2
         AlmostEquals(m.Row(3), { 0, 0, 0, 1 });
}

void Dump(const mat4& m) {
  const float* x = m.a_;
  fprintf(
      stdout,
      "{{%9.3f %9.3f %9.3f %9.3f}\n"
      " {%9.3f %9.3f %9.3f %9.3f}\n"
      " {%9.3f %9.3f %9.3f %9.3f}\n"
      " {%9.3f %9.3f %9.3f %9.3f}}\n",
      x[0],
      x[1],
      x[2],
      x[3],
      x[4],
      x[5],
      x[6],
      x[7],
      x[8],
      x[9],
      x[10],
      x[11],
      x[12],
      x[13],
      x[14],
      x[15]);
}
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("mat4 mul") {
    mat4 a = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53 };
    mat4 b = { 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131 };
    mat4 c;

    Mul(a, b, &c);

    mat4 d = { 1585, 1655, 1787, 1861, 5318, 5562, 5980, 6246, 10514, 11006, 11840, 12378, 15894, 16634, 17888, 18710 };

    ASSERT_TRUE(Equals(c, d));
  }

  TEST_CASE("mat4 transpose") {
    mat4 m = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    Transpose(&m, &m);

    ASSERT_TRUE(Equals(m.v_[0], { 1, 5, 9, 13 }));
    ASSERT_TRUE(Equals(m.v_[1], { 2, 6, 10, 14 }));
    ASSERT_TRUE(Equals(m.v_[2], { 3, 7, 11, 15 }));
    ASSERT_TRUE(Equals(m.v_[3], { 4, 8, 12, 16 }));
  }
}