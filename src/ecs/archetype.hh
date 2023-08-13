#pragma once

#include "../common/hash-map.hh"
#include "../common/list.hh"

#include "chunk.hh"
#include "component-registry.hh"

namespace game {
struct EntityQuery;

struct Archetype;
struct ArchetypeChunkData;

// The first time an entity of a particular archetype is added to the chunk it will be added to this data
struct ArchetypeChunkData {
  // This is just a bunch of arrays concatenated after each other
  // this has been done to minimize the number of allocations

  // Chunk* chunks_[];
  // u32    change_version_[];
  // u32    entity_count_[];
  // shared component data could go here...

  void* ptr_;
  i32   len_;
  i32   cap_;
  i32   component_count_;       // Same as Archetype::component_count_ (the number of component types in the archetype)
  i32   chunk_entity_capacity_; // Same as Archetype::chunk_entity_capacity_

  // ---

  void Create(i32 component_count, i32 chunk_entity_capacity);

  void Destroy();

  // ---

  i32 Len() const { return len_; }

  i32 Cap() const { return cap_; }

  void Add(Chunk* chunk, u32 change_version);

  // ---

  Chunk** ChunkPtrArray() const { return (Chunk**)ptr_; }

  i32 _ChunkPtrArraySize() const { return i32(sizeof(Chunk**)) * cap_; }

  // Each component type has it's own change version in the chunk metadata
  u32* ChangeVersionArray() const { return (u32*)((byte*)ptr_ + _ChunkPtrArraySize()); }

  // Find the change version array for a sequence of chunks based on the component type index for the archetype
  u32* ChangeVersionArray(i32 archetype_component_type_index) const {
    return ChangeVersionArray() + archetype_component_type_index * cap_;
  }

  i32 _ChangeVersionArraySize() const { return 4 * component_count_ * cap_; }

  i32* EntityCountArray() const { return (i32*)((byte*)ptr_ + _ChunkPtrArraySize() + _ChangeVersionArraySize()); }

  i32 _EntityCountArraySize() const { return 4 * cap_; }

  // ---

  // The total allocated bytes of this buffer
  i32 _BufferSize() const {
    return _ChunkPtrArraySize() + _ChunkPtrArraySize() + _ChangeVersionArraySize() + _EntityCountArraySize();
  }
};

struct Archetype {
  ComponentTypeId*   types_;
  i32                types_len_;
  u16*               sizes_;                  // size of each component
  i32*               offsets_;                // offset to component data array in chunk
  i32                chunk_entity_capacity_;  // Maximum number of entities per chunk
  ArchetypeChunkData chunk_data_;             // chunks?
  List<Chunk*>       chunk_with_empty_slots_; // Chunk free list (chunks that have space)
  i32                entity_count_;           // number of entities allocated of this archetype
  const char*        label_;
  List<EntityQuery*> matching_queries_;

  // The type identity of an archetype is a sorted set of component types
  Slice<const ComponentTypeId> TypeId() const { return { types_, types_len_, types_len_ }; }

  void Destroy() {
    chunk_data_.Destroy();
    chunk_with_empty_slots_.Destroy();
    matching_queries_.Destroy();
  }
};

struct ArchetypeListMap {
  HashMap<int>     map_;
  List<Archetype*> list_;

  void Create(int initial_capacity) {
    MemZeroInit(this);
    map_.Create(MEM_ALLOC_HEAP, initial_capacity);
    list_.Create(MEM_ALLOC_HEAP, initial_capacity);
  }

  void Destroy() {
    list_.Destroy();
    map_.Destroy();
  }

  // ---

  // May return null
  Archetype* TryGet(Slice<const ComponentTypeId> types);

  void Add(Archetype* archetype);
};
} // namespace game