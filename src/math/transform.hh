#pragma once

#include "data.hh"

// We have adopted the DirectXMath convention
//
// - matrices are row major (translation in 4th row, element 13, 14, 15)
// - vectors are row vectors (x'=xA)
// - we use pre-multiplication to combine transforms (ABC = first A then B lastly C)
//
// DirectXMath does not dictate a specific handedness
// but we always use left handed coordinate systems

namespace game {
// we should not use this directly... because it does not flip the camera around when we go behind the object
// void LookToLH(vec3 eye, vec3 fw, vec3 up, mat4* m);

// Construct a world-to-view matrix from a camera position, camera focus/target position and an up direction (normal).
void LookAtLH(const vec3& eye_pos, const vec3& focus_pos, const vec3& up_dir, mat4* m);

// Orthographic projection
void OrthoLH(float left, float right, float bottom, float top, float near, float far, mat4* m);

void PerspectiveFovLH(float fov, float aspect, float near, float far, mat4* m);

void RotationX(float angle, mat4* m);
void RotationY(float angle, mat4* m);
void RotationZ(float angle, mat4* m);

// todo: translation

// Transform point (w=1) or direction (w=0)
vec4 Transform(const mat4& m, const vec4& v);

// Like TransformPoint but with perspective divide
vec4 TransformPerspective(const mat4& proj, const vec4& v);
} // namespace game
