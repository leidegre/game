#include "math.hh"

#include <cstdio>

namespace {
using namespace game;
using namespace math;

bool AreEqual(const mat3& a, const mat3& b, float tolerance = 0.001f) {

  vec3 c0 = Abs(a.c0 - b.c0);
  vec3 c1 = Abs(a.c1 - b.c1);
  vec3 c2 = Abs(a.c2 - b.c2);

  return c0 < tolerance && c1 < tolerance && c2 < tolerance;
}

bool AreEqual(const mat4& a, const mat4& b, float tolerance = 0.001f) {
  vec4 c0 = Abs(a.c0 - b.c0);
  vec4 c1 = Abs(a.c1 - b.c1);
  vec4 c2 = Abs(a.c2 - b.c2);
  vec4 c3 = Abs(a.c3 - b.c3);

  return c0 < tolerance && c1 < tolerance && c2 < tolerance && c3 < tolerance;
}

void Dump(const mat3& m) {
  printf(
      "{{%9.3ff, %9.3ff, %9.3ff},\n"
      " {%9.3ff, %9.3ff, %9.3ff},\n"
      " {%9.3ff, %9.3ff, %9.3ff}}\n",
      m.c0.x,
      m.c0.y,
      m.c0.z,
      m.c1.x,
      m.c1.y,
      m.c1.z,
      m.c2.x,
      m.c2.y,
      m.c2.z);
}

const char* ToString(const mat4& m) {
  static char tmp[512];
  sprintf(
      tmp,
      "{{ %9.3ff, %9.3ff, %9.3ff, %9.3ff },\n"
      " { %9.3ff, %9.3ff, %9.3ff, %9.3ff },\n"
      " { %9.3ff, %9.3ff, %9.3ff, %9.3ff },\n"
      " { %9.3ff, %9.3ff, %9.3ff, %9.3ff }}\n",
      m.c0.x,
      m.c1.x,
      m.c2.x,
      m.c3.x,
      m.c0.y,
      m.c1.y,
      m.c2.y,
      m.c3.y,
      m.c0.z,
      m.c1.z,
      m.c2.z,
      m.c3.z,
      m.c0.w,
      m.c1.w,
      m.c2.w,
      m.c3.w);
  return tmp;
}

void Dump(const mat4& m) {
  puts(ToString(m));
}

void Dump(const vec4& v) {
  printf("{ %9.3ff, %9.3ff, %9.3ff, %9.3ff }\n", v.x, v.y, v.z, v.w);
}
} // namespace

#define ASSERT_MAT4(expected, actual)                                                                                  \
  do {                                                                                                                 \
    test_assert_metadata _metadata = { __FILE__, __LINE__, #expected, #actual };                                       \
    if (!AreEqual(expected, actual)) {                                                                                 \
      test_report_failure();                                                                                           \
      fputs("expected\n", stderr);                                                                                     \
      Dump(expected);                                                                                                  \
      fputs("actual\n", stderr);                                                                                       \
      Dump(actual);                                                                                                    \
      fputs("delta (actual - expected)\n", stderr);                                                                    \
      mat4 tmp;                                                                                                        \
      tmp.c0 = actual.c0 - expected.c0;                                                                                \
      tmp.c1 = actual.c1 - expected.c1;                                                                                \
      tmp.c2 = actual.c2 - expected.c2;                                                                                \
      tmp.c3 = actual.c3 - expected.c3;                                                                                \
      Dump(tmp);                                                                                                       \
      test_assertion_fail();                                                                                           \
      return 1;                                                                                                        \
    }                                                                                                                  \
  } while (0)