#pragma once

#include "data.hh"

// https://learn.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline

namespace game {
namespace math {
// The two input vectors are assumed to be unit length and not collinear.
inline mat3 LookRotation(const vec3& forward, const vec3& up) {
  vec3 t = Normalize(Cross(up, forward));
  mat3 m = { t, Cross(forward, t), forward };
  return m;
}

inline mat4 LookAtLH(const vec3& eye, const vec3& target, const vec3& up) {
  vec3 look_towards = target - eye;

  vec3 r2 = Normalize(look_towards);
  vec3 r0 = Normalize(Cross(up, r2));
  vec3 r1 = Cross(r2, r0);

  f32 d0 = -Dot(r0, eye);
  f32 d1 = -Dot(r1, eye);
  f32 d2 = -Dot(r2, eye);

  mat4 m;
  m.c0 = { r0.x, r0.y, r0.z, d0 };
  m.c1 = { r1.x, r1.y, r1.z, d1 };
  m.c2 = { r2.x, r2.y, r2.z, d2 };
  m.c3 = { 0, 0, 0, 1 };

  return m;
}

inline mat4 OrthographicLH(f32 width, f32 height, f32 near, f32 far) {
  float f_range = 1.0f / (far - near);

  mat4 m = {
    {2.0f / width,          0.0f,     0.0,            0.0f},
    {        0.0f, 2.0f / height,     0.0,            0.0f},
    {        0.0f,          0.0f, f_range, -f_range * near},
    {        0.0f,          0.0f,     0.0,            1.0f}
  };
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

inline mat4 PerspectiveFovLH(f32 vertical_fov, f32 aspect, f32 near, f32 far) {
  // https://learn.microsoft.com/en-us/windows/win32/direct3d9/projection-transform

  f32 sin_fovy = sinf(0.5f * vertical_fov);
  f32 cos_fovy = cosf(0.5f * vertical_fov);
  f32 h        = cos_fovy / sin_fovy; // cot(x) = cos(x)/sin(x) = 1/tan(x)
  f32 w        = h / aspect;
  f32 f_range  = far / (far - near); // Q

  const vec4 m[4] = {
    {   w, 0.0f,            0.0f, 0.0f},
    {0.0f,    h,            0.0f, 0.0f},
    {0.0f, 0.0f,         f_range, 1.0f},
    {0.0f, 0.0f, -near * f_range, 0.0f},
  };

  return mat4::FromRowData(m);
}

inline mat4 Translation(const vec3& translation) {
  mat4 m;
  m.c0 = { 1, 0, 0, 0 };
  m.c1 = { 0, 1, 0, 0 };
  m.c2 = { 0, 0, 1, 0 };
  m.c3 = translation.xyz1();
  return m;
}

inline mat4 ScaleUniform(f32 s) {
  mat4 m;
  m.c0 = { s, 0, 0, 0 };
  m.c1 = { 0, s, 0, 0 };
  m.c2 = { 0, 0, s, 0 };
  m.c3 = { 0, 0, 0, 1 };
  return m;
}

// Combine translation, rotation and scale into a transformation matrix
inline mat4 TRS(const vec3& translation, const quat& rotation, const vec3& scale) {
  mat3 r = ToMat3(rotation);
  mat4 m;
  m.c0 = xyz0(scale.x * r.c0);
  m.c1 = xyz0(scale.y * r.c1);
  m.c2 = xyz0(scale.z * r.c2);
  m.c3 = translation.xyz1();
  return m;
}

// matrix multiplication with column vector
inline vec4 Transform(const mat4& a, const vec4& b) {
  return a.c0 * b.x + a.c1 * b.y + a.c2 * b.z + a.c3 * b.w;
}
} // namespace math
} // namespace game
