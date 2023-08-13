#pragma once

#include "../common/type-system.hh"

#include <cmath>

// Math data structures without code (maybe move these into the type system?)

namespace game {
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

  vec3 yzx() const { return { y, z, x }; }
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

  vec3 yxw() const { return { y, x, w }; }
  vec3 zwx() const { return { z, w, x }; }
  vec3 wzy() const { return { w, z, y }; }
};

inline vec4 operator+(const vec4& a, const vec4& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

// scalar multiplication
inline vec4 operator*(float s, const vec4& a) {
  return { s * a.x, s * a.y, s * a.z, s * a.w };
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

  vec3 c0; // column 0
  vec3 c1; // column 1
  vec3 c2; // column 2
};

// Each column of the matrix is continuous in memory (column-major).
struct alignas(64) mat4 {
  static const mat4& Identity() {
    static const mat4 identity = {
      1.0f, 0.0f, 0.0f, 0.0f, // col 0
      0.0f, 1.0f, 0.0f, 0.0f, // col 1
      0.0f, 0.0f, 1.0f, 0.0f, // col 2
      0.0f, 0.0f, 0.0f, 1.0f, // col 3
    };
    return identity;
  }

  vec4 c0; // column 0
  vec4 c1; // column 1
  vec4 c2; // column 2
  vec4 c3; // column 3
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
} // namespace game
