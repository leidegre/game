#pragma once

#include "data.hh"

namespace game {
inline vec3 operator+(const vec3& a, const vec3& b) {
  vec3 tmp;
  tmp.v_[0] = a.v_[0] + b.v_[0];
  tmp.v_[1] = a.v_[1] + b.v_[1];
  tmp.v_[2] = a.v_[2] + b.v_[2];
  return tmp;
}

inline vec3 operator-(const vec3& a, const vec3& b) {
  vec3 tmp;
  tmp.v_[0] = a.v_[0] - b.v_[0];
  tmp.v_[1] = a.v_[1] - b.v_[1];
  tmp.v_[2] = a.v_[2] - b.v_[2];
  return tmp;
}

// Negate vector (flip) [use case???]
// inline vec3 operator-(const vec3& v) {
//   vec3 tmp;
//   tmp.v_[0] = -v.v_[0];
//   tmp.v_[1] = -v.v_[1];
//   tmp.v_[2] = -v.v_[2];
//   return tmp;
// }

// Scale length
inline vec3 operator*(const float s, const vec3& v) {
  vec3 tmp;
  tmp.v_[0] = s * v.v_[0];
  tmp.v_[1] = s * v.v_[1];
  tmp.v_[2] = s * v.v_[2];
  return tmp;
}

//???
// inline vec3 operator/(const float s, const vec3& v) {
//   vec3 tmp;
//   tmp.v_[0] = s / v.v_[0];
//   tmp.v_[1] = s / v.v_[1];
//   tmp.v_[2] = s / v.v_[2];
//   return tmp;
// }

inline float Length(const vec3& v) {
  return sqrtf(v.v_[0] * v.v_[0] + v.v_[1] * v.v_[1] + v.v_[2] * v.v_[2]);
}

inline vec3 Normalize(const vec3& v) {
  return (1.f / Length(v)) * v;
}

inline float Dot(const vec3& a, const vec3& b) {
  auto x = a.v_;
  auto y = b.v_;
  return x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
}

inline vec3 Cross(const vec3& a, const vec3& b) {
  const float* x = &a.v_[0];
  const float* y = &b.v_[0];
  vec3         tmp;
  tmp.v_[0] = x[1] * y[2] - x[2] * y[1];
  tmp.v_[1] = x[2] * y[0] - x[0] * y[2];
  tmp.v_[2] = x[0] * y[1] - x[1] * y[0];
  return tmp;
}
} // namespace game
