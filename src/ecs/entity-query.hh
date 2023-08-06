#pragma once

#include "component.hh"

#include "../common/list.hh"

namespace game {
struct Archetype;
struct Chunk;
struct EntityManager;

// What was this for?
struct EntityQueryMask {
  enum {
    MAX_CAPACITY      = 1024,
    MAX_CAPACITY_BYTE = MAX_CAPACITY / 8,
  };
  byte           index_;
  byte           mask_;
  EntityManager* entity_manager_; // ???
};

// An entity query is just a place where we keep track of archetypes (and it's chunks) that have matching component types
struct EntityQuery {
  ComponentTypeId*           all_;
  ComponentDataAccess::Mode* all_access_mode_;
  i32                        all_len_;

  ComponentTypeId*           any_;
  ComponentDataAccess::Mode* any_access_mode_;
  i32                        any_len_;

  ComponentTypeId*           none_;
  ComponentDataAccess::Mode* none_access_mode_;
  i32                        none_len_;

  Slice<ComponentTypeId> All() {
    return { all_, all_len_, all_len_ };
  }

  Slice<ComponentDataAccess::Mode> AllAccessMode() {
    return { all_access_mode_, all_len_, all_len_ };
  }

  Slice<ComponentTypeId> Any() {
    return { any_, any_len_, any_len_ };
  }

  Slice<ComponentDataAccess::Mode> AnyAccessMode() {
    return { any_access_mode_, any_len_, any_len_ };
  }

  Slice<ComponentTypeId> None() {
    return { none_, none_len_, none_len_ };
  }

  Slice<ComponentDataAccess::Mode> NoneAccessMode() {
    return { none_access_mode_, none_len_, none_len_ };
  }

  EntityQueryMask mask_;

  List<Archetype*> matching_archetypes_;
  List<Chunk*>     matching_chunks_; // the chunk cache is rebuilt when the cache has been invalidated

  // ---

  void Destroy() {
    matching_archetypes_.Destroy();
    matching_chunks_.Destroy();
  }

  // ---

  bool IsMatch(Archetype* archetype);

  // ---

  u32 HashCode();

  bool Equals(const EntityQuery& other);
};
} // namespace game
