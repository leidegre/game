#pragma once

#include "type-system.hh"

namespace game {
struct ComponentRegistry {
  Slice<const TypeInfo> components_;

  void Initialize(Slice<const TypeInfo> components) {
    assert(components[0].type_id_.v_ == 0);
    assert(components[0].size_ == int(sizeof(Entity)));
    assert(components[0].alignment_ == int(alignof(Entity)));
    for (int i = 1; i < components.Len(); i++) {
      assert(components[i].type_id_.Index() == i && "fatal: invalid component registry");
    }
    components_ = components;
  }

  // ---

  const TypeInfo* GetComponentTypeInfo(ComponentTypeId component_type_id) {
    return &components_[component_type_id.Index()];
  }
};
} // namespace game
