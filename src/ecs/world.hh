#pragma once

#include "system.hh"

namespace game {
struct World {
  ComponentRegistry* type_registry_;
  ChunkAllocator*    chunk_allocator_;
  EntityManager*     entity_manager_;
  List<System*>      system_list_;
  List<SystemState>  system_state_;

  void Create(Slice<const TypeInfo> components);

  void Destroy();

  // ---

  EntityManager& EntityManager() {
    // Note sure about this... we're just trying to minimize the number of pointers really
    // Access good we can check stuff here and put breakpoints, dot notation over arrow notation?
    return *entity_manager_;
  }

  // ---

  void Register(System* system) {
    system_list_.Add(system);
    SystemState state{};
    state.entity_manger_ = entity_manager_;
    system_state_.Add(state);
  }

  void Update();
};
} // namespace game