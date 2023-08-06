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

struct alignas(4) vec2 {
  f32 v_[2];

  f32 x() const {
    return v_[0];
  }

  f32 y() const {
    return v_[1];
  }
};

struct alignas(4) vec3 {
  f32 v_[3];

  f32 x() const {
    return v_[0];
  }

  f32 y() const {
    return v_[1];
  }

  f32 z() const {
    return v_[2];
  }
};

inline vec3 operator*(float s, const vec3& v) {
  vec3 tmp;
  tmp.v_[0] = s * v.v_[0];
  tmp.v_[1] = s * v.v_[1];
  tmp.v_[2] = s * v.v_[2];
  return tmp;
}

// 4-component vector of floating point values
struct alignas(16) vec4 {
  f32 v_[4];

  f32 x() const {
    return v_[0];
  }

  f32 y() const {
    return v_[1];
  }

  f32 z() const {
    return v_[2];
  }

  f32 w() const {
    return v_[3];
  }
};

// ---
// Swizzle
// ---

// Extend 2d-vector to { x, y, 0, 1 }
inline vec4 xyzw(const vec2& v) {
  return { v.x(), v.y(), 0, 1 };
}

// Extend 3d-vector to { x, y, z, 1 }
inline vec4 xyzw(const vec3& v) {
  return { v.x(), v.y(), v.z(), 1 };
}

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

    return { v.x(), v.y(), v.z(), cos_h };
  }

  f32 v_[4];
};

// Each row of the matrix is continuous in memory (row-major).
struct alignas(16) mat4 {
  static const mat4& Identity() {
    static const mat4 identity = {
      1.0f, 0.0f, 0.0f, 0.0f, // row 0
      0.0f, 1.0f, 0.0f, 0.0f, // row 1
      0.0f, 0.0f, 1.0f, 0.0f, // row 2
      0.0f, 0.0f, 0.0f, 1.0f, // row 3
    };
    return identity;
  }

  union {
    f32  m_[4][4];
    f32  a_[16];
    vec4 v_[4];
  };

  const vec4& Row(int i) const {
    return this->v_[i];
  }

  void SetRow(int i, const vec4& row) {
    this->v_[i] = row;
  }

  vec4 Col(int j) const {
    vec4 col;
    col.v_[0] = a_[j];
    col.v_[1] = a_[4 + j];
    col.v_[2] = a_[8 + j];
    col.v_[3] = a_[12 + j];
    return col;
  }

  void SetCol(int j, const vec4& col) {
    a_[j]      = col.v_[0];
    a_[4 + j]  = col.v_[1];
    a_[8 + j]  = col.v_[2];
    a_[12 + j] = col.v_[3];
  }

  // ---

  static void Translation(const vec3& xyz, mat4* t) {
    t->SetRow(0, { 1, 0, 0, 0 });
    t->SetRow(1, { 0, 1, 0, 0 });
    t->SetRow(2, { 0, 0, 1, 0 });
    t->SetRow(3, xyzw(xyz));
  }

  // Rotation matrix from quaternion
  static void Rotation(const quat& q, mat4* t) {
    t->SetRow(0, { 1, 0, 0, 0 });
    t->SetRow(1, { 0, 1, 0, 0 });
    t->SetRow(2, { 0, 0, 1, 0 });
    t->SetRow(3, { 0, 0, 0, 1 });
  }

  // Uniform scale
  static void Scale(f32 s, mat4* t) {
    t->SetRow(0, { s, 0, 0, 0 });
    t->SetRow(1, { 0, s, 0, 0 });
    t->SetRow(2, { 0, 0, s, 0 });
    t->SetRow(3, { 0, 0, 0, 1 });
  }
};

// ---

namespace bitwise {
union FloatInt {
  f32 f_;
  i32 v_;
};

inline i32 ToInt(f32 f) {
  FloatInt u;
  u.f_ = f;
  return u.v_;
}

inline u32 ToUInt(f32 f) {
  return (u32)ToInt(f);
}

inline f32 ToFloat(i32 v) {
  FloatInt u;
  u.v_ = v;
  return u.f_;
}

inline f32 ToFloat(u32 v) {
  return ToFloat((i32)v);
}
}; // namespace bitwise
} // namespace game