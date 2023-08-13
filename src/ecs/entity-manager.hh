#pragma once

#include "../common/mem-block.hh"

#include "archetype.hh"
#include "component-registry.hh" // aka ecs/types.hh
#include "entity-query.hh"

namespace game {
struct World;
struct ChunkAllocator;
struct EntityManager;

// Note that the meaning of this will depend on the value of chunk
// When chunk is null the index is absolute (entity index)
// When chunk is not null the index is relative (index of entity in chunk)
struct _ChunkEntityIndex {
  Chunk* chunk_;
  i32    index_; // absolute (entity index) or relative (index of entity in chunk)
};

// A slice of the entities from a chunk
struct _ChunkEntitySlice {
  Chunk* chunk_;
  i32    index_; // Index of first entity in chunk
  i32    count_; // Number of entities in this range
};

struct EntityManager {
  World*            world_;
  ArchetypeListMap  archetypes_;
  MemBlockAllocator archetype_allocator_; // allocator used to allocate archetypes and entity queries
  i32               entity_capacity_;     // max number of entities that can currently be allocated

  i32                next_free_entity_index_;
  u32                entity_create_destroy_version_; // Updated each time an entity is created or destroyed
  u32*               version_by_entity_;
  _ChunkEntityIndex* entity_chunk_index_by_entity_;
  Archetype**        archetype_by_entity_; // how is this important?

  Archetype* entity_archetype_;

  Archetype* CreateArchetype(Slice<const ComponentTypeId> types);
  Archetype* CreateArchetype(std::initializer_list<ComponentTypeId> types) {
    return CreateArchetype(slice::FromInitializer(types));
  }

  // ---
  // Entity API
  // ---

  // Creates a new entity.
  void CreateEntity(Archetype* archetype, Entity* entities, i32 count);

  // Creates a new entity without any components.
  Entity CreateEntity() {
    Entity entity;
    CreateEntity(entity_archetype_, &entity, 1);
    return entity;
  }
  // Creates a new entity of a specific archetype.
  Entity CreateEntity(Archetype* archetype) {
    Entity entity;
    CreateEntity(archetype, &entity, 1);
    return entity;
  }

  void DestroyEntities(Entity* entities, i32 count);

  void DestroyEntity(Entity entity) { DestroyEntities(&entity, 1); }

  // // Copies an existing entity and creates a new entity from that copy.
  // void Instantiate();
  // // Destroys an existing entity.
  // void DestroyEntity();
  // // Adds a component to an existing entity.
  // void AddComponent();
  // // 	Removes a component from an existing entity.
  // void RemoveComponent();
  // // Retrieves the value of an entity's component.
  // void GetComponent();
  // // Overwrites the value of an entity's component.
  // void SetComponent();

  // ---

  void Create(World* world, i32 initial_capacity);

  void Destroy();

  void _SetCapacity(i32 new_capacity);

  _ChunkEntitySlice _FindFirstEntityRange(Entity* entities, i32 count);

  // ---

  HashMap<i32>       query_map_; // Maps hashes of queries to indicies in query list
  List<EntityQuery*> query_list_;
  i32                query_mask_count_;

  // Entity queries track archetypes with matching component types
  // Entity queries are built from query descriptors that tell us what component types are to be read/written/excluded in the query
  EntityQuery* CreateQuery(const ComponentDataAccess* query_desc, i32 query_desc_len);
  EntityQuery* CreateQuery(std::initializer_list<ComponentDataAccess> query_desc) {
    return CreateQuery(query_desc.begin(), i32(query_desc.size()));
  }
};
} // namespace game