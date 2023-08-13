#pragma once

#include "data.hh"

namespace game {
namespace math {
inline float RSqrt(float x) {
  // https://en.wikipedia.org/wiki/Fast_inverse_square_root
  // https://arxiv.org/pdf/2307.15600.pdf
  // https://twitter.com/mike_acton/status/1685896474025177088
  return 1.0f / sqrtf(x);
}

inline vec3 Abs(const vec3& v) {
  return { fabs(v.x), fabs(v.y), fabs(v.z) };
}

//hmm... putting an operator in a namespace...
inline bool operator<(const vec3& v, float other) {
  return v.x < other && v.y < other && v.z < other;
}

inline f32 Dot(const vec3& a, const vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 Normalize(const vec3& v) {
  return RSqrt(Dot(v, v)) * v;
}

inline vec3 Cross(const vec3& x, const vec3& y) {
  return (x * y.yzx() - y * x.yzx()).yzx();
}
} // namespace math

// Convert quaternion to rotation matrix
inline mat3 ToMat3(const quat& q) {
  using namespace bitwise;

  vec4 v  = q.v_;
  vec4 v2 = v + v;

  vec3u npn = { 0x80000000, 0x00000000, 0x80000000 };
  vec3u nnp = { 0x80000000, 0x80000000, 0x00000000 };
  vec3u pnn = { 0x00000000, 0x80000000, 0x80000000 };

  mat3 m;
  m.c0 = v2.y * ToFloat(ToUInt(v.yxw()) ^ npn) - v2.z * ToFloat(ToUInt(v.zwx()) ^ pnn) + vec3{ 1, 0, 0 };
  m.c1 = v2.z * ToFloat(ToUInt(v.wzy()) ^ nnp) - v2.x * ToFloat(ToUInt(v.yxw()) ^ npn) + vec3{ 0, 1, 0 };
  m.c2 = v2.x * ToFloat(ToUInt(v.zwx()) ^ pnn) - v2.y * ToFloat(ToUInt(v.wzy()) ^ nnp) + vec3{ 0, 0, 1 };
  return m;
}

// Transpose matrix in place
inline void Transpose(mat3* m) {
  vec3 c0 = m->c0;
  vec3 c1 = m->c1;
  vec3 c2 = m->c2;

  m->c0 = { c0.x, c1.x, c2.x };
  m->c1 = { c0.y, c1.y, c2.y };
  m->c2 = { c0.z, c1.z, c2.z };
}

// Transpose matrix in place
inline void Transpose(mat4* m) {
  vec4 c0 = m->c0;
  vec4 c1 = m->c1;
  vec4 c2 = m->c2;
  vec4 c3 = m->c3;

  m->c0 = { c0.x, c1.x, c2.x, c3.x };
  m->c1 = { c0.y, c1.y, c2.y, c3.y };
  m->c2 = { c0.z, c1.z, c2.z, c3.z };
  m->c3 = { c0.w, c1.w, c2.w, c3.w };
}

inline mat4 Mul(const mat4& a, const mat4& b) {
  mat4 tmp = {
    b.c0.x * a.c0 + b.c0.y * a.c1 + b.c0.z * a.c2 + b.c0.w * a.c3,
    b.c1.x * a.c0 + b.c1.y * a.c1 + b.c1.z * a.c2 + b.c1.w * a.c3,
    b.c2.x * a.c0 + b.c2.y * a.c1 + b.c2.z * a.c2 + b.c2.w * a.c3,
    b.c3.x * a.c0 + b.c3.y * a.c1 + b.c3.z * a.c2 + b.c3.w * a.c3,
  };
  return tmp;
};
} // namespace game