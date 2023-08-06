#pragma once

#include "../common/type-system.hh"

// Define component for inclusion in registry
#define GAME_COMPONENT(Component)                                                                                      \
  { ::game::GetComponentTypeId<Component>(), u16(sizeof(Component)), u16(alignof(Component)), #Component }

namespace game {
struct Entity {
  enum { COMPONENT_TYPE = 0 }; // builtin

  i32 index_;
  u32 version_;
};

struct ComponentTypeId {
  u32 v_;

  i32 Index() const {
    return i32(v_);
  }

  // ---

  bool operator==(const ComponentTypeId& other) const {
    return this->v_ == other.v_;
  }

  bool operator<(const ComponentTypeId& other) const {
    return this->v_ < other.v_;
  }
};

// Get the component type. The low bits is a unique index for the component type. The high bits track component kind, i.e. data, buffer, tag, shared or chunk.
template <typename T> constexpr ComponentTypeId GetComponentTypeId() {
  return ComponentTypeId{ T::COMPONENT_TYPE };
};

// ---

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

  bool Any() const {
    return (access_mode_ & ANY) == ANY;
  }

  bool Exclude() const {
    return (access_mode_ & EXCLUDE) == EXCLUDE;
  }

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
} // namespace game