#pragma once

#include "data.hh"

namespace game {
inline quat Quat(const mat4& m) {
  return quat::Identity();
}
} // namespace game