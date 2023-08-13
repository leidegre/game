#pragma once

#include "math.hh"

namespace game {
namespace math {
// The two input vectors are assumed to be unit length and not collinear.
inline mat3 LookRotation(const vec3& forward, const vec3& up) {
  vec3 t = Normalize(Cross(up, forward));
  mat3 m = { t, Cross(forward, t), forward };
  return m;
}

inline mat4 LookAt(const vec3& eye, const vec3& target, const vec3& up) {
  mat3 rot = LookRotation(Normalize(target - eye), up);
  mat4 m;
  m.c0 = xyz0(rot.c0);
  m.c1 = xyz0(rot.c1);
  m.c2 = xyz0(rot.c2);
  return m;
}

inline mat4 Ortho(f32 width, f32 height, f32 near, f32 far) {
  f32 rcpdx = 1.0f / width;
  f32 rcpdy = 1.0f / height;
  f32 rcpdz = 1.0f / (far - near);

  mat4 m = {
    {2.0f * rcpdx,         0.0f,          0.0f,                  0.0f},
    {        0.0f, 2.0f * rcpdy,          0.0f,                  0.0f},
    {        0.0f,         0.0f, -2.0f * rcpdz, -(far + near) * rcpdz},
    {        0.0f,         0.0f,          0.0f,                  1.0f},
  };
  return m;
}

inline mat4 PerspectiveFov(f32 verticalFov, f32 aspect, f32 near, f32 far) {
  f32 cotangent = 1.0f / tanf(verticalFov * 0.5f);
  f32 rcpdz     = 1.0f / (near - far);

  mat4 m = {
    {cotangent / aspect,      0.0f,                 0.0f,                      0.0f},
    {              0.0f, cotangent,                 0.0f,                      0.0f},
    {              0.0f,      0.0f, (far + near) * rcpdz, 2.0f * near * far * rcpdz},
    {              0.0f,      0.0f,                -1.0f,                      0.0f},
  };
  return m;
}

// Combine translation, rotation and scale into a transformation matrix
inline mat4 TRS(const vec3& translation, const quat& rotation, const vec3& scale) {
  mat3 r = ToMat3(rotation);
  mat4 m;
  m.c0 = xyz0(scale.x * r.c0);
  m.c1 = xyz0(scale.y * r.c1);
  m.c2 = xyz0(scale.z * r.c2);
  m.c3 = xyz1(translation);
  return m;
}
} // namespace math
} // namespace game
