#pragma once

#include "../math/data.hh"

// Define component
#define GAME_COMPONENT(Component)                                                                                      \
  { ::game::GetComponentTypeId<Component>(), u16(sizeof(Component)), u16(alignof(Component)), #Component }

namespace game {
struct Entity {
  enum { COMPONENT_TYPE = 0 }; // builtin

  i32 index_; // -1 is invalid
  u32 version_;
};

// A named type of a global component type ID (absolute index). To access the index value use the Index() member function.
struct ComponentTypeId {
  u32 v_;

  i32 Index() const { return i32(v_); }

  // ---

  bool operator==(const ComponentTypeId& other) const { return this->v_ == other.v_; }

  bool operator<(const ComponentTypeId& other) const { return this->v_ < other.v_; }
};

// Get the component type. The low bits is a unique index for the component type. The high bits track component kind, i.e. data, buffer, tag, shared or chunk.
template <typename T> constexpr ComponentTypeId GetComponentTypeId() {
  return ComponentTypeId{ T::COMPONENT_TYPE };
};

struct TypeInfo {
  ComponentTypeId type_id_;
  u16             size_;
  u16             alignment_;
  const char*     name_;
};
} // namespace game