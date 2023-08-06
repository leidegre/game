#include "transform.hh"

#include "mat4.hh"
#include "vec3.hh"

using namespace game;

constexpr float F32_MIN_VALUE = 1.f / 65536.f; // log10(0.000015259)≈-4.8

static void game__LookToLH(const vec3& eye_pos, const vec3& eye_dir, const vec3& up_dir, mat4* m) {
  // https://github.com/microsoft/DirectXMath/blob/73db2f18ec2b076e231c2267511a71b773bc5a12/Inc/DirectXMathMatrix.inl#L2214-L2240

  // this can be true if we move through the focus point
  // CHECK(!((eye_dir.v_[0] == 0) & (eye_dir.v_[1] == 0) & (eye_dir.v_[2] == 0)));

  // CHECK(isfinite(eye_dir.v_[0]));
  // CHECK(isfinite(eye_dir.v_[1]));
  // CHECK(isfinite(eye_dir.v_[1]));

  // CHECK(!((up_dir.v_[0] == 0) & (up_dir.v_[1] == 0) & (up_dir.v_[2] == 0)));

  // CHECK(isfinite(up_dir.v_[0]));
  // CHECK(isfinite(up_dir.v_[1]));
  // CHECK(isfinite(up_dir.v_[1]));

  auto r2 = Normalize(eye_dir);
  auto r0 = Normalize(Cross(up_dir, r2));
  auto r1 = Cross(r2, r0);

  auto d0 = -Dot(r0, eye_pos);
  auto d1 = -Dot(r1, eye_pos);
  auto d2 = -Dot(r2, eye_pos);

  m->v_[0] = vec4{ r0.v_[0], r0.v_[1], r0.v_[2], d0 };
  m->v_[1] = vec4{ r1.v_[0], r1.v_[1], r1.v_[2], d1 };
  m->v_[2] = vec4{ r2.v_[0], r2.v_[1], r2.v_[2], d2 };
  m->v_[3] = vec4{ 0, 0, 0, 1 };

  // if the view matrix is a orthogonal matrix
  // and the view matrix must transform the world
  // to view space; we want the inverse view matrix
  // to do that

  Transpose(m, m); // this is inverting the matrix since it is orthogonal
}

void game::LookAtLH(const vec3& eye_pos, const vec3& focus_pos, const vec3& up_dir, mat4* m) {
  // It is possible to build a right handed look at matrix by flipping
  // the position of focus_pos and eye_pos but we don't use right handed
  // coordinate systems
  //
  // history lesson: I thought I could use the LookToLH function directly
  //                 but that doesn't work because it doesn't adjust the
  //                 forward direction with respect to moving the camera
  //                 around, you end up with a really stupid looking world
  //                 and I spent a long time troubleshooting this

  vec3 eye_dir = focus_pos - eye_pos;

  game__LookToLH(eye_pos, eye_dir, up_dir, m);
}

void game::OrthoLH(float left, float right, float bottom, float top, float near, float far, mat4* m) {
  // https://github.com/microsoft/DirectXMath/blob/73db2f18ec2b076e231c2267511a71b773bc5a12/Inc/DirectXMathMatrix.inl#L3020-L3044
  // https://github.com/g-truc/glm/blob/b3f87720261d623986f164b2a7f6a0a938430271/glm/ext/matrix_clip_space.inl#L18-L25

  // This is a translation combined with a scale to match NDC, not that we use glClipControl(..., GL_ZERO_TO_ONE) to match Direct3D and Vulkan conventions

  // this is the XMMatrixOrthographicOffCenterLH

  // CHECK(F32_MIN_VALUE < fabsf(right - left));
  // CHECK(F32_MIN_VALUE < fabsf(top - bottom));
  // CHECK(F32_MIN_VALUE < fabsf(far - near));

  const float inv_width  = 1.0f / (right - left);
  const float inv_height = 1.0f / (top - bottom);
  const float f_range    = 1.0f / (far - near);

  const float ortho[4][4] = {
    {      inv_width + inv_width,                            0,               0,    0},
    {                          0,      inv_height + inv_height,               0,    0},
    {                          0,                            0,         f_range,    0},
    {-(left + right) * inv_width, -(top + bottom) * inv_height, -f_range / near, 1.0f},
  };

  mat4_copy(m, ortho);
}

void game::PerspectiveFovLH(float fov_y, float aspect, float near, float far, mat4* m) {
  // https://github.com/microsoft/DirectXMath/blob/73db2f18ec2b076e231c2267511a71b773bc5a12/Inc/DirectXMathMatrix.inl#L2446-L2481

  // these are quite annoying
  // CHECK(0.f < near && 0.f < far);
  // CHECK(F32_MIN_VALUE < fabsf(fov));
  // CHECK(F32_MIN_VALUE < fabsf(aspect));
  // CHECK(F32_MIN_VALUE < fabsf(far - near));

  // maybe: https://github.com/microsoft/DirectXMath/blob/332fb99d07d64f3b4fa89fafb4f303b758420749/Inc/DirectXMathMisc.inl#L2304-L2352
  const float sin_fov = sinf(0.5f * fov_y);
  const float cos_fov = cosf(0.5f * fov_y);

  const float height  = cos_fov / sin_fov;
  const float width   = height / aspect;
  const float f_range = far / (far - near);

  const float perspective[4][4] = {
    {width,      0,               0, 0},
    {    0, height,               0, 0},
    {    0,      0,         f_range, 1},
    {    0,      0, -f_range * near, 0},
  };

  mat4_copy(m, perspective);
}

// From the book "Game Engine Architecture"
//
// - The 1 within the upper 3 × 3 always appears on the axis we’re rotating about,
//   while the sine and cosine terms are off-axis
//
// - Positive rotations go from x to y (about z), from y to z (about x), and from
//   z to x (about y). The z to x rotation “wraps around,” which is why the
//   rotation matrix about the y-axis is transposed relative to the other two.
//
// - The inverse of a pure rotation is just its transpose.

void game::RotationX(float angle, mat4* m) {
  // The GEA book has the negative sign opposite of
  // the Wikipedia article on "Rotation matrix"
  // I'm not sure why that is... I should derive
  // these equations...

  const float sin = sinf(angle);
  const float cos = cosf(angle);

  const float rot[4][4] = {
    {1,    0,   0, 0},
    {0,  cos, sin, 0},
    {0, -sin, cos, 0},
    {0,    0,   0, 1},
  };

  mat4_copy(m, rot);
}

void game::RotationY(float angle, mat4* m) {
  const float sin = sinf(angle);
  const float cos = cosf(angle);

  const float rot[4][4] = {
    {cos, 0, -sin, 0},
    {  0, 1,    0, 0},
    {sin, 0,  cos, 0},
    {  0, 0,    0, 1}
  };

  mat4_copy(m, rot);
}

void game::RotationZ(float angle, mat4* m) {
  const float sin = sinf(angle);
  const float cos = cosf(angle);

  const float rot[4][4] = {
    { cos, sin, 0, 0},
    {-sin, cos, 0, 0},
    {   0,   0, 1, 0},
    {   0,   0, 0, 1}
  };

  mat4_copy(m, rot);
}

vec4 game::Transform(const mat4& m, const vec4& v) {
  // is this right? does this matter? is this unambiguous?

  // plug this into Wolfram Alpha
  // {{a,b,c,d},{e,f,g,h},{i,j,k,l},{m,n,o,p}}{{x},{y},{z},{w}}

  // and you will get this back
  // (a x + b y + c z + d w
  //  e x + f y + g z + h w
  //  i x + j y + k z + l w
  //  m x + n y + o z + p w)

  vec4 temp;

  temp.v_[0] = m.m_[0][0] * v.v_[0] + m.m_[0][1] * v.v_[1] + m.m_[0][2] * v.v_[2] + m.m_[0][3] * v.v_[3];
  temp.v_[1] = m.m_[1][0] * v.v_[0] + m.m_[1][1] * v.v_[1] + m.m_[1][2] * v.v_[2] + m.m_[1][3] * v.v_[3];
  temp.v_[2] = m.m_[2][0] * v.v_[0] + m.m_[2][1] * v.v_[1] + m.m_[2][2] * v.v_[2] + m.m_[2][3] * v.v_[3];
  temp.v_[3] = m.m_[3][0] * v.v_[0] + m.m_[3][1] * v.v_[1] + m.m_[3][2] * v.v_[2] + m.m_[3][3] * v.v_[3];

  return temp;
}

vec4 game::TransformPerspective(const mat4& proj, const vec4& v) {
  // https://www.youtube.com/watch?v=EqNcqBdrNyI

  vec4 r = Transform(proj, v);

  r.v_[0] /= r.v_[3];
  r.v_[1] /= r.v_[3];
  r.v_[2] /= r.v_[3];

  return r;
}