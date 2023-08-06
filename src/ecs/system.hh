#pragma once

#include "../common/type-system.hh"

namespace game {
struct EntityManager;

struct SystemState {
  EntityManager* entity_manger_;
  EntityManager& EntityManager() {
    return *entity_manger_;
  }

  f32 dT_; // delta time in seconds since last frame
};

struct System {
  virtual void OnCreate(SystemState& state) {
    //...
  }
  virtual void OnUpdate(SystemState& state) {
    //...
  }
  virtual void OnDestroy(SystemState& state) {
    //...
  }
};
} // namespace game
