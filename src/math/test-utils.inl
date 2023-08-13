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

void Dump(const mat3& m) {
  printf(
      "{{%9ff, %9ff, %9ff},\n"
      " {%9ff, %9ff, %9ff},\n"
      " {%9ff, %9ff, %9ff}}\n",
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
} // namespace