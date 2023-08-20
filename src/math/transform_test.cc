#include "transform.hh"

#include "math-test-utils.inl"

#include <DirectXMath.h>

#include <cstring>

#include "../test/test.h"

using namespace game;
using namespace math;

namespace {
// convert from XMMATRIX row-major to mat4 column-major
mat4 ToMat4(DirectX::XMMATRIX dxm) {
  dxm = DirectX::XMMatrixTranspose(dxm);
  mat4 m;
  memcpy(&m, &dxm, 64);
  return m;
}

vec4 ToVec4(DirectX::XMVECTOR v) {
  return { v.m128_f32[0], v.m128_f32[1], v.m128_f32[2], v.m128_f32[3] };
}

// Convert a 3D position into a homogeneous coordinate
DirectX::FXMVECTOR ToDirectXVector(const vec3& v) {
  return { v.x, v.y, v.z, 1 };
}
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("Dump") {
    mat4 m = {
      { 1,  2,  3,  4}, // column 0
      { 5,  6,  7,  8}, // column 1
      { 9, 10, 11, 12}, // column 2
      {13, 14, 15, 16}, // column 3
    };

    // The output should look like this:

    const char* expected = "{{     1.000f,     5.000f,     9.000f,    13.000f },\n"
                           " {     2.000f,     6.000f,    10.000f,    14.000f },\n"
                           " {     3.000f,     7.000f,    11.000f,    15.000f },\n"
                           " {     4.000f,     8.000f,    12.000f,    16.000f }}\n";

    const char* actual = ToString(m);

    if (!(strcmp(expected, actual) == 0)) {
      puts(actual);
      ASSERT_TRUE(false);
    }
  }

  TEST_CASE("LookRotation(0)") {
    mat3 expected = {
      { 0.948683f, 0.000000f, -0.316228f},
      {-0.169031f, 0.845154f, -0.507093f},
      { 0.267261f, 0.534522f,  0.801784f}
    };

    vec3 forward = Normalize(vec3{ 1.0f, 2.0f, 3.0f });
    vec3 up      = { 0.0f, 1.0f, 0.0f };
    mat3 actual  = LookRotation(forward, up);

    ASSERT_TRUE(AreEqual(expected, actual));
  }

  TEST_CASE("LookRotation(1)") {
    mat3 expected = {
      { 0.447214f, 0.000000f, -0.894427f},
      {-0.717137f, 0.597614f, -0.358569f},
      { 0.534522f, 0.801784f,  0.267261f}
    };

    vec3 forward = Normalize(vec3{ 2.0f, 3.0f, 1.0f });
    vec3 up      = { 0.0f, 1.0f, 0.0f };
    mat3 actual  = LookRotation(forward, up);

    ASSERT_TRUE(AreEqual(expected, actual));
  }

  TEST_CASE("LookRotation(2)") {
    mat3 expected = {
      { 0.832050f, 0.000000f, -0.554700f},
      {-0.148250f, 0.963624f, -0.222375f},
      { 0.534522f, 0.267261f,  0.801784f}
    };

    vec3 forward = Normalize(vec3{ 2.0f, 1.0f, 3.0f });
    vec3 up      = { 0.0f, 1.0f, 0.0f };
    mat3 actual  = LookRotation(forward, up);

    ASSERT_TRUE(AreEqual(expected, actual));
  }

  TEST_CASE("Quaternion to Rotation Matrix") {
    ASSERT_TRUE(AreEqual(mat3::Identity(), ToMat3(quat::Identity())));
  }

  TEST_CASE("Coordinate System") {
    // Here we use the DirectX math library to check
    // that our own math library does the right thing

    // The DirectX math library use row-major matrix layout
    // therefore we transpose the XMMATRIX to make it column-major
    // Also the XMMatrixMultiply is different for the same reason

    mat4              view    = LookAtLH({ 0, 0, 1 }, { 0, 0, 0 }, { 0, 1, 0 });
    DirectX::XMMATRIX dx_view = DirectX::XMMatrixLookAtLH({ 0, 0, 1 }, { 0, 0, 0 }, { 0, 1, 0 });
    ASSERT_TRUE(AreEqual(ToMat4(dx_view), view));

    // ---

    mat4 proj = math::PerspectiveFovLH(HALF_PI, 1280.0f / 800.0f, 0.1f, 100.0f);

    DirectX::XMMATRIX dx_proj = DirectX::XMMatrixPerspectiveFovLH(HALF_PI, 1280.0f / 800.0f, 0.1f, 100.0f);

    ASSERT_TRUE(AreEqual(ToMat4(dx_proj), proj));

    // ---

    DirectX::XMMATRIX dx_view_proj = DirectX::XMMatrixMultiply(dx_view, dx_proj);

    mat4 view_proj = Mul(proj, view);

    // These matrices won't be equal since matrix multiplication is not commutative
    // However, if we transform a position v the following should hold
    // v*dx_view_proj = view_proj*v

    // DirectX::XMVector3TransformCoord({ 0, 0, 0 }, dx_view_proj);
    // ASSERT_MAT4(ToMat4(dx_view_proj), view_proj);
  }

  TEST_CASE("LookAtLH") {
    struct Camera {
      vec3 eye_;
      vec3 target_;
      vec3 up_;
    };

    Camera camera[] = {
      {{ 0, 0, -1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 1, 0, -1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 0, 1, -1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 1, 1, -1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 0, 0, +1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 1, 0, +1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 0, 1, +1 }, { 0, 0, 0 }, { 0, 1, 0 }},
      {{ 1, 1, +1 }, { 0, 0, 0 }, { 0, 1, 0 }},
    };

    for (i32 i = 0; i < ArrayLength(camera); i++) {
      mat4 view = LookAtLH(camera[i].eye_, camera[i].target_, camera[i].up_);

      DirectX::FXMVECTOR dx_eye    = ToDirectXVector(camera[i].eye_);
      DirectX::FXMVECTOR dx_target = ToDirectXVector(camera[i].target_);
      DirectX::FXMVECTOR dx_up     = ToDirectXVector(camera[i].up_);
      DirectX::XMMATRIX  dx_view   = DirectX::XMMatrixLookAtLH(dx_eye, dx_target, dx_up);

      ASSERT_MAT4(ToMat4(dx_view), view);
    }
  }

  TEST_CASE("OrthographicLH") {
    struct Orthographic {
      f32 width_;
      f32 height_;
      f32 near_;
      f32 far_;
    };

    Orthographic ortho[] = {
      {1280.0f, 800.0f, 0.1f,  10.0f},
      {1280.0f, 800.0f, 1.0f,  10.0f},
      {1280.0f, 800.0f, 0.1f, 100.0f},
      {1280.0f, 800.0f, 1.0f, 100.0f},
    };

    for (i32 i = 0; i < ArrayLength(ortho); i++) {
      mat4 proj = OrthographicLH(ortho[i].width_, ortho[i].height_, ortho[i].near_, ortho[i].far_);

      DirectX::XMMATRIX dx_proj =
          DirectX::XMMatrixOrthographicLH(ortho[i].width_, ortho[i].height_, ortho[i].near_, ortho[i].far_);

      ASSERT_MAT4(ToMat4(dx_proj), proj);
    }
  }

  TEST_CASE("PerspectiveFovLH") {
    struct Perspective {
      f32 fovy_;
      f32 aspect_;
      f32 near_;
      f32 far_;
    };

    Perspective perspective[] = {
      {HALF_PI, 1280.0f / 800.0f, 0.1f,  10.0f},
      {HALF_PI,             1.0f, 0.1f,  10.0f},
      {HALF_PI,             1.0f, 1.0f,  10.0f},
      {     PI,             1.0f, 1.0f,  10.0f},
      {HALF_PI, 1280.0f / 800.0f, 0.1f, 100.0f},
      {HALF_PI,             1.0f, 0.1f, 100.0f},
      {HALF_PI,             1.0f, 1.0f, 100.0f},
      {     PI,             1.0f, 1.0f, 100.0f},
    };

    for (i32 i = 0; i < ArrayLength(perspective); i++) {
      mat4 proj =
          PerspectiveFovLH(perspective[i].fovy_, perspective[i].aspect_, perspective[i].near_, perspective[i].far_);

      DirectX::XMMATRIX dx_proj = DirectX::XMMatrixPerspectiveFovLH(
          perspective[i].fovy_, perspective[i].aspect_, perspective[i].near_, perspective[i].far_);

      ASSERT_MAT4(ToMat4(dx_proj), proj);
    }
  }

  TEST_CASE("TransformClip") {
    putc('\n', stdout);

    puts("math");
    mat4 proj = PerspectiveFovLH(HALF_PI, 1280.0f / 800.0f, 0.1f, 10.0f);
    Dump(proj);

    // XMVECTOR Z = XMVectorSplatZ(V); // make ZZZZ
    // XMVECTOR Y = XMVectorSplatY(V); // make YYYY
    // XMVECTOR X = XMVectorSplatX(V); // make XXXX

    // XMVECTOR Result = XMVectorMultiply(Z, M.r[2]); // componentwise multiplication
    // Result = XMVectorMultiplyAdd(Y, M.r[1], Result);  // componentwise multiplication + addition
    // Result = XMVectorMultiplyAdd(X, M.r[0], Result);

    vec4 v1 = math::Transform(proj, { 0, 0, 0.1f, 1 });
    Dump(v1);

    vec4 v2 = v1 * (1.0f / v1.w);
    Dump(v2);

    vec4 v3 = math::Transform(proj, { 0, 0, 10.0f, 1 });
    Dump(v3);

    vec4 v4 = v3 * (1.0f / v3.w);
    Dump(v4);

    puts("DirectX");
    DirectX::XMMATRIX dx_proj = DirectX::XMMatrixPerspectiveFovLH(HALF_PI, 1280.0f / 800.0f, 0.1f, 10.0f);
    Dump(ToMat4(dx_proj));

    DirectX::XMVECTOR dx_v1 = DirectX::XMVector3TransformNormal({ 0, 0, 0.1f }, dx_proj);
    Dump(ToVec4(dx_v1));
    DirectX::XMVECTOR dx_v2 = DirectX::XMVector3TransformCoord({ 0, 0, 0.1f }, dx_proj);
    Dump(ToVec4(dx_v2));

    DirectX::XMVECTOR dx_v3 = DirectX::XMVector3TransformNormal({ 0, 0, 10.0f }, dx_proj);
    Dump(ToVec4(dx_v3));
    DirectX::XMVECTOR dx_v4 = DirectX::XMVector3TransformCoord({ 0, 0, 10.0f }, dx_proj);
    Dump(ToVec4(dx_v4));
  }
}