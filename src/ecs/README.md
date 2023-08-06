# Component registry

We need to know the memory layout of each component before we can use them.

```cpp
struct Entity {
  enum { COMPONENT_TYPE = 0 }; // builtin

  i32 index_;
  u32 version_;
};

#define GAME_COMPONENT(Component)                                                                                      \
  { ::game::GetComponentTypeId<Component>(), i32(sizeof(Component)), i32(alignof(Component)), #Component }

static const TypeInfo type_info[] = {
  GAME_COMPONENT(Entity),
};
```

Components are defined in JavaScript will which generate all the metadata. This way we can inspect all component state.

# Entities

Entities with a specific set of component types belong to a particular archetype.

To create an entity you need an archetype.

Structural changes require synchronization and is best done after frame has rendered.

# MoveForward

- A tag component, i.e. zero size component
- Component query, A, B, C r/w, "subtractive" exclude entity if it has a particular component
- ComponentArray
- Control system ordering is nice to have...
