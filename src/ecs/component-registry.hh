#pragma once

#include "component.hh"

namespace game {
struct TypeInfo {
  ComponentTypeId type_id_;
  u16             size_;
  u16             alignment_;
  const char*     name_;

  // all components that store entities must store them in a single `Entity array[]`

  int entity_offset_count_;       // for components that store entities?
  int entity_offset_start_index_; // for components that store entities?
  int buffer_cap_;                // for buffer components
  int max_chunk_cap_;
  // int memory_ordering_ // ???
  // int stable_type_hash_ // ??? probably to detect type change at runtime
};

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
