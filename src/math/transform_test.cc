#include "transform.hh"

#include "test-utils.inl"

#include "../test/test.h"

using namespace game;
using namespace math;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

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
}