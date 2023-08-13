#pragma once

#include "system.hh"

#include "../math/math.hh"

namespace game {
struct EntityQuery;

struct TRS_LocalToWorldSystem : public System {
  EntityQuery* q_;

  void OnCreate(SystemState& state) override;

  void OnUpdate(SystemState& state) override;
};
} // namespace game