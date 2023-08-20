#pragma once

// Math data structures

#include "../common/type-system.hh"

#include <cmath>

namespace game {
struct vec3;
struct vec4;

// If the difference between two floating point values is less than this value we consider them to be "almost" equal
const f32 EPSILON = 1e-5f;

const f32 PI      = 3.14159265359;
const f32 TWO_PI  = 6.28318530718;
const f32 HALF_PI = 1.57079632679;

// ---

struct alignas(4) vec3u {
  u32 x;
  u32 y;
  u32 z;
};

inline vec3u operator^(const vec3u& a, const vec3u& b) {
  return { a.x ^ b.x, a.y ^ b.y, a.z ^ b.z };
}

// ---

struct alignas(4) vec3 {
  f32 x;
  f32 y;
  f32 z;

  // Reinterpret the vector as an array of length 3
  f32 (&Array())[3] { return *reinterpret_cast<f32(*)[3]>(this); }

  vec3 yzx() const { return { y, z, x }; }

  // point to homogeneous coordinate { y, z, x, 1 }
  vec4 xyz1() const;
};

inline vec3 operator+(const vec3& a, const vec3& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline vec3 operator-(const vec3& a, const vec3& b) {
  return { a.x - b.x, a.y - b.y, a.z - b.z };
}

// scalar multiplication
inline vec3 operator*(float s, const vec3& v) {
  vec3 tmp;
  tmp.x = s * v.x;
  tmp.y = s * v.y;
  tmp.z = s * v.z;
  return tmp;
}

// componentwise multiplication
inline vec3 operator*(const vec3& x, const vec3& y) {
  return { x.x * y.x, x.y * y.y, x.z * y.z };
}

// 4-component vector of floating point values
struct alignas(16) vec4 {
  f32 x;
  f32 y;
  f32 z;
  f32 w;

  // Reinterpret the vector as an array of length 4
  f32 (&Array())[4] { return *reinterpret_cast<f32(*)[4]>(this); }
  const f32 (&Array() const)[4] { return *reinterpret_cast<const f32(*)[4]>(this); }

  vec3 yxw() const { return { y, x, w }; }
  vec3 zwx() const { return { z, w, x }; }
  vec3 wzy() const { return { w, z, y }; }
};

// vector addition
inline vec4 operator+(const vec4& a, const vec4& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

// vector subtraction
inline vec4 operator-(const vec4& a, const vec4& b) {
  return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

// scalar multiplication
inline vec4 operator*(float s, const vec4& a) {
  return { s * a.x, s * a.y, s * a.z, s * a.w };
}

// scalar multiplication
inline vec4 operator*(const vec4& a, float s) {
  return { a.x * s, a.y * s, a.z * s, a.w * s };
}

// componentwise multiplication
inline vec4 operator*(const vec4& a, const vec4& b) {
  return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

// ---
// Swizzle
// ---

// Extend 3d-vector to { x, y, z, 1 }
inline vec4 xyz1(const vec3& v) {
  return { v.x, v.y, v.z, 1 };
}

// Extend 3d-vector to { x, y, z, 0 }
inline vec4 xyz0(const vec3& v) {
  return { v.x, v.y, v.z, 1 };
}

// Shrink 4d-vector to { x, y, z }
inline vec3 xyz(const vec4& v) {
  return { v.x, v.y, v.z };
}

inline vec4 vec3::xyz1() const {
  return { x, y, z, 1 };
}

// ---

// Quaternion
struct alignas(16) quat {
  static const quat& Identity() {
    static const quat identity = { 0.0f, 0.0f, 0.0f, 1.0f };
    return identity;
  }

  static quat FromAxisAngle(const vec3& axis, f32 angle) {
    f32  h     = 0.5f * angle;
    f32  sin_h = sinf(h);
    f32  cos_h = cosf(h);
    vec3 v     = sin_h * axis;

    return { v.x, v.y, v.z, cos_h };
  }

  vec4 v_;
};

// Each column of the matrix is continuous in memory (column-major).
struct alignas(4) mat3 {
  static const mat3& Identity() {
    static const mat3 identity = {
      1.0f, 0.0f, 0.0f, // col 0
      0.0f, 1.0f, 0.0f, // col 1
      0.0f, 0.0f, 1.0f, // col 2
    };
    return identity;
  }

  // Reinterpret the matrix as an array of 3 columns
  vec3 (&Array())[3] { return *reinterpret_cast<vec3(*)[3]>(this); }

  vec3 c0; // column 0
  vec3 c1; // column 1
  vec3 c2; // column 2
};

// Each column of the matrix is continuous in memory (column-major).
struct alignas(64) mat4 {
  static const mat4& Identity() {
    static const mat4 identity = {
      1.0f, 0.0f, 0.0f, 0.0f, // column 0
      0.0f, 1.0f, 0.0f, 0.0f, // column 1
      0.0f, 0.0f, 1.0f, 0.0f, // column 2
      0.0f, 0.0f, 0.0f, 1.0f, // column 3
    };
    return identity;
  }

  // Reinterpret the matrix as an array of 4 columns
  vec4 (&Column())[4] { return *reinterpret_cast<vec4(*)[4]>(this); }
  const vec4 (&Column() const)[4] { return *reinterpret_cast<const vec4(*)[4]>(this); }

  vec4 Row(i32 row_index) const {
    vec4 row;
    row.x = c0.Array()[row_index];
    row.y = c1.Array()[row_index];
    row.z = c2.Array()[row_index];
    row.w = c3.Array()[row_index];
    return row;
  }

  vec4 c0; // column 0
  vec4 c1; // column 1
  vec4 c2; // column 2
  vec4 c3; // column 3

  // Construct matrix from row-major notation
  static mat4 FromRowData(const vec4 (&r)[4]) {
    // This is a transpose
    mat4 tmp;
    tmp.c0 = { r[0].x, r[1].x, r[2].x, r[3].x };
    tmp.c1 = { r[0].y, r[1].y, r[2].y, r[3].y };
    tmp.c2 = { r[0].z, r[1].z, r[2].z, r[3].z };
    tmp.c3 = { r[0].w, r[1].w, r[2].w, r[3].w };
    return tmp;
  }
};

// ---

namespace bitwise {
union FloatInt {
  f32 f_;
  i32 v_;
};

// Bitwise conversion of 32-bit floating point number to signed 32-bit integer
inline i32 ToInt(f32 f) {
  FloatInt u;
  u.f_ = f;
  return u.v_;
}

// Bitwise conversion of 32-bit floating point number to unsigned 32-bit integer
inline u32 ToUInt(f32 f) {
  return (u32)ToInt(f);
}

// Bitwise conversion of signed 32-bit integer to 32-bit floating point number
inline f32 ToFloat(i32 v) {
  FloatInt u;
  u.v_ = v;
  return u.f_;
}

// Bitwise conversion of unsigned 32-bit integer to 32-bit floating point number
inline f32 ToFloat(u32 v) {
  return ToFloat((i32)v);
}

// ---

// Bitwise conversion
inline vec3u ToUInt(const vec3& v) {
  return { ToUInt(v.x), ToUInt(v.y), ToUInt(v.z) };
}

// Bitwise conversion
inline vec3 ToFloat(const vec3u& v) {
  return { ToFloat(v.x), ToFloat(v.y), ToFloat(v.z) };
}
}; // namespace bitwise

// ---

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

inline vec4 Abs(const vec4& v) {
  return { fabs(v.x), fabs(v.y), fabs(v.z), fabs(v.w) };
}

//hmm... putting an operator in a namespace...
inline bool operator<(const vec3& v, float other) {
  return v.x < other && v.y < other && v.z < other;
}

inline bool operator<(const vec4& v, float other) {
  return v.x < other && v.y < other && v.z < other && v.w < other;
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

inline mat4 Transpose(mat4 m) {
  vec4 c0 = m.c0;
  vec4 c1 = m.c1;
  vec4 c2 = m.c2;
  vec4 c3 = m.c3;

  m.c0 = { c0.x, c1.x, c2.x, c3.x };
  m.c1 = { c0.y, c1.y, c2.y, c3.y };
  m.c2 = { c0.z, c1.z, c2.z, c3.z };
  m.c3 = { c0.w, c1.w, c2.w, c3.w };

  return m;
}

// Note that mat4 is column-major. The transformation A will be applied after B.
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
