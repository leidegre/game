#pragma once

#include "type-system.hh"

#include "../common/list.hh"

namespace game {
struct Archetype;
struct Chunk;
struct EntityManager;

struct ComponentDataAccess {
  enum Mode : byte {
    READ_ONLY       = 0,
    READ_WRITE      = 1,
    READ_WRITE_MASK = READ_ONLY | READ_WRITE,
    ANY             = 2, // At least one of the component types in this array must exist in the archetype
    ANY_READ_ONLY   = ANY | READ_ONLY,
    ANY_READ_WRITE  = ANY | READ_WRITE,
    EXCLUDE         = 4, // None of the component types in this array can exist in the archetype
  };

  ComponentTypeId type_id_;
  Mode            access_mode_; // Bitfield (tells us how you intend to access the component data)

  bool Any() const { return (access_mode_ & ANY) == ANY; }

  bool Exclude() const { return (access_mode_ & EXCLUDE) == EXCLUDE; }

  template <typename T> static ComponentDataAccess Read() {
    return ComponentDataAccess{ GetComponentTypeId<T>(), READ_ONLY };
  }

  template <typename T> static ComponentDataAccess ReadAny() {
    return ComponentDataAccess{ GetComponentTypeId<T>(), ANY_READ_ONLY };
  }

  // Write implies read and/or write
  template <typename T> static ComponentDataAccess Write() {
    return ComponentDataAccess{ GetComponentTypeId<T>(), READ_WRITE };
  }

  template <typename T> static ComponentDataAccess Exclude() {
    return ComponentDataAccess{ GetComponentTypeId<T>(), EXCLUDE };
  }
};

inline bool operator<(const ComponentDataAccess& x, const ComponentDataAccess& y) {
  if (x.type_id_ == y.type_id_) {
    return x.access_mode_ < y.access_mode_;
  }
  return x.type_id_ < y.type_id_;
}

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

// An entity query is an expression of a data dependency.
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

  Slice<ComponentTypeId> All() { return { all_, all_len_, all_len_ }; }

  Slice<ComponentDataAccess::Mode> AllAccessMode() { return { all_access_mode_, all_len_, all_len_ }; }

  Slice<ComponentTypeId> Any() { return { any_, any_len_, any_len_ }; }

  Slice<ComponentDataAccess::Mode> AnyAccessMode() { return { any_access_mode_, any_len_, any_len_ }; }

  Slice<ComponentTypeId> None() { return { none_, none_len_, none_len_ }; }

  Slice<ComponentDataAccess::Mode> NoneAccessMode() { return { none_access_mode_, none_len_, none_len_ }; }

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

  // count the number of entities matched by query
  i32 Count();
};
} // namespace game
