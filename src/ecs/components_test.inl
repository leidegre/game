#include "../math/data.hh"

namespace {
struct Translation {
  enum { COMPONENT_TYPE = 1 };

  vec3 value_;
};

struct Rotation {
  enum { COMPONENT_TYPE = 2 };

  quat value_;
};

struct Scale {
  enum { COMPONENT_TYPE = 3 };

  f32 value_;
};

struct LocalToWorld {
  enum { COMPONENT_TYPE = 4 };

  mat4 value_;
};
} // namespace