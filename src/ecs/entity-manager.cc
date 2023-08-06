#include "entity-manager.hh"

#include "archetype.hh"
#include "chunk.hh"
#include "entity-query.hh"

#include "../common/mem.hh"

using namespace game;

void EntityManager::Create(World* world, i32 initial_capacity) {
  MemZeroInit(this);

  world_ = world;

  archetypes_.Create(64);

  archetype_allocator_.Create();

  _SetCapacity(initial_capacity);

  query_map_.Create(MEM_ALLOC_HEAP, 0);
  query_list_.Create(MEM_ALLOC_HEAP, 0);

  // Setup built-in entity only archetype

  entity_archetype_         = CreateArchetype({ nullptr, 0, 0 });
  entity_archetype_->label_ = "Entity";
}

void EntityManager::Destroy() {
  for (auto query : query_list_) {
    query->Destroy();
  }

  query_map_.Destroy();
  query_list_.Destroy();

  // This is just the reverse of what the _SetCapacity function does

  if (entity_chunk_index_by_entity_ != nullptr) {
    MemFree(MEM_ALLOC_HEAP, entity_chunk_index_by_entity_);
    entity_chunk_index_by_entity_ = nullptr;
  }

  if (archetype_by_entity_ != nullptr) {
    MemFree(MEM_ALLOC_HEAP, archetype_by_entity_);
    archetype_by_entity_ = nullptr;
  }

  if (version_by_entity_ != nullptr) {
    MemFree(MEM_ALLOC_HEAP, version_by_entity_);
    version_by_entity_ = nullptr;
  }

  // Destroy all archetypes before we destroy the archetype allocator

  for (auto archetype : archetypes_.list_) {
    archetype->Destroy();
  }

  archetypes_.Destroy();

  archetype_allocator_.Destroy();

  world_ = nullptr;
}

Archetype* EntityManager::CreateArchetype(Slice<const ComponentTypeId> unsorted_types) {
  auto sorted_types = MemStackalloc(ComponentTypeId, 0, unsorted_types.Len() + 1);
  sorted_types      = Append(sorted_types, GetComponentTypeId<Entity>());
  for (auto type : unsorted_types) {
    sorted_types = Insert(sorted_types, type);
  }

#if _DEBUG
  for (int i = 1; i < sorted_types.len_; i++) {
    assert(sorted_types[i - 1] < sorted_types[i]);
  }
#endif

  auto existing_archetype = archetypes_.TryGet(sorted_types.Const());
  if (existing_archetype != nullptr) {
    return existing_archetype;
  }

  Archetype* new_archetype = archetype_allocator_.Allocate<Archetype>();

  MemZeroInit(new_archetype);

  auto types   = archetype_allocator_.AllocateArray<ComponentTypeId>(sorted_types.Len());
  auto sizes   = archetype_allocator_.AllocateArray<u16>(sorted_types.Len());
  auto offsets = archetype_allocator_.AllocateArray<i32>(sorted_types.Len());

  new_archetype->types_     = MemCopyArray(types, sorted_types.ptr_, sorted_types.ByteLen());
  new_archetype->types_len_ = sorted_types.Len();

  for (int i = 0; i < sorted_types.len_; i++) {
    const TypeInfo* type_info = world_->type_registry_->GetComponentTypeInfo(sorted_types[i]);

    sizes[i] = type_info->size_;
  }

  new_archetype->sizes_ = sizes;

  // ---

  i32 max_chunk_entity_capacity = CHUNK_BUFFER_SIZE / i32(sizeof(Entity));

  auto GetComponentArraySize = [](i32 component_size, i32 entity_count) -> i32 {
    return MemAlign(component_size * entity_count, MEM_CACHE_LINE_SIZE);
  };

  auto CalculateSpaceRequirement =
      [GetComponentArraySize](uint16_t* component_sizes, i32 component_count, i32 entity_count) -> i32 {
    i32 size = 0;
    for (i32 i = 0; i < component_count; i++) {
      size += GetComponentArraySize(component_sizes[i], entity_count);
    }
    return size;
  };

  auto CalculateChunkCapacity = [CalculateSpaceRequirement](uint16_t* component_sizes, i32 count) -> i32 {
    i32 total_size = 0;
    for (i32 i = 0; i < count; i++) {
      total_size += component_sizes[i];
    }
    // guess
    i32 capacity = CHUNK_BUFFER_SIZE / total_size;
    // adjust
    while (!(CalculateSpaceRequirement(component_sizes, count, capacity) < CHUNK_BUFFER_SIZE)) {
      capacity--;
    }
    return capacity;
  };

  new_archetype->chunk_entity_capacity_ =
      Min(CalculateChunkCapacity(sizes, sorted_types.Len()), max_chunk_entity_capacity);

  assert(
      (0 < new_archetype->chunk_entity_capacity_)
      & (new_archetype->chunk_entity_capacity_ <= max_chunk_entity_capacity));

  int used_bytes = 0; // relative chunk buffer

  for (int i = 0; i < sorted_types.Len(); i++) {
    const TypeInfo* type_info = world_->type_registry_->GetComponentTypeInfo(sorted_types[i]);

    offsets[i] = used_bytes;

    used_bytes += MemAlign(type_info->size_ * new_archetype->chunk_entity_capacity_, MEM_CACHE_LINE_SIZE);
  }

  new_archetype->offsets_ = offsets;

  new_archetype->chunk_with_empty_slots_ = List<Chunk*>::WithAllocator(MEM_ALLOC_HEAP);

  new_archetype->matching_queries_ = List<EntityQuery*>::WithAllocator(MEM_ALLOC_HEAP);

  // ---

  archetypes_.Add(new_archetype);

  return new_archetype;
}

void EntityManager::CreateEntity(Archetype* archetype, Entity* entities, i32 count) {
  // ...

  for (; 0 < count;) {
    // make entities
    Chunk* chunk;

    if (0 < archetype->chunk_with_empty_slots_.Len()) {
      chunk = archetype->chunk_with_empty_slots_[0];
    } else {
      chunk = nullptr;
    }

    if (chunk == nullptr) {
      // todo: All assignments/initialization of chunk header should stay in scope here and not be spread out over multiple functions

      chunk                     = world_->chunk_allocator_->Allocate();
      chunk->header_.archetype_ = archetype;
      chunk->header_.len_       = 0;
      chunk->header_.cap_       = archetype->chunk_entity_capacity_;
      archetype->chunk_data_.Add(chunk, 42); // todo: change version

      chunk->header_.free_list_index_ = archetype->chunk_with_empty_slots_.Len();
      archetype->chunk_with_empty_slots_.Add(chunk);
    }

    assert(chunk->EntityCount() < chunk->EntityCapacity());

    Entity* chunk_entities_end = chunk->EntityArray() + chunk->EntityCount();

    const i32 n = Min(count, chunk->EntityCapacity() - chunk->EntityCount());

    for (i32 i = 0; i < n; i++) {
      Entity* entity = chunk_entities_end + i;

      // Initially entities are created as an ascending sequence of indexes
      // but when entities are destroyed it will create holes and instead of
      // "compacting" the array we move the last index into the hole to be
      // reused

      // the purpose of this is two fold, we don't create more indexes than we need
      // and we try to fill holes as soon as possible

      const i32 entity_index = next_free_entity_index_;

      i32 next_free_entity_index = entity_chunk_index_by_entity_[entity_index].index_;
      if (next_free_entity_index == -1) {
        _SetCapacity(2 * entity_capacity_);
        next_free_entity_index = entity_chunk_index_by_entity_[entity_index].index_;
      }
      next_free_entity_index_ = next_free_entity_index;

      // ---

      u32 version = version_by_entity_[entity_index];

      entity->index_   = entity_index;
      entity->version_ = version;

      _ChunkEntityIndex* entity_chunk_index = entity_chunk_index_by_entity_ + entity_index;
      entity_chunk_index->chunk_            = chunk;
      entity_chunk_index->index_            = chunk->EntityCount() + i;

      entity_create_destroy_version_++;

      // optional
      if (entities != nullptr) {
        entities->index_   = entity_index;
        entities->version_ = version;
        entities++;
      }
    }

    chunk->AddEntityCount(n);

    if (chunk->EntityCount() == chunk->EntityCapacity()) {
      // This chunk has now been filled up. We must therefore remove it from the "chunks with space" list

      archetype->chunk_with_empty_slots_.RemoveAtSwapBack(chunk->header_.free_list_index_);
    }

    archetype->entity_count_ += n;

    count -= n;
  }
}

_ChunkEntitySlice EntityManager::_FindFirstEntityRange(Entity* entities, i32 count) {
  assert(0 < count);

  // Look for continuous ranges of entities from the same chunk (this operation is order dependant)

  const u32*         versions       = version_by_entity_;
  _ChunkEntityIndex* chunk_indicies = entity_chunk_index_by_entity_;

  // ---

  i32                base_entity_index = entities[0].index_;
  _ChunkEntityIndex* base_chunk_index  = chunk_indicies + base_entity_index;
  Chunk* const base_chunk = versions[base_entity_index] == entities[0].version_ ? base_chunk_index->chunk_ : nullptr;
  i32 const    base_chunk_entity_index = base_chunk_index->index_;

  i32 chunk_entity_count = 0;
  for (; chunk_entity_count < count; chunk_entity_count++) {
    const Entity* entity         = entities + chunk_entity_count; // handle to destroy
    const i32     entity_index   = entity->index_;
    const u32     entity_version = entity->version_;

    _ChunkEntityIndex* chunk_index        = chunk_indicies + entity_index;
    Chunk* const       chunk              = chunk_index->chunk_;
    i32                chunk_entity_index = chunk_index->index_; // index of entity in chunk

    if (versions[entity_index] == entity_version) {
      // Entity handle is still valid
      if (!(base_chunk == chunk)) {
        break;
      }
      // why would the indicies not match here? and if they don't why is that not an error?
    } else {
      // Version mismatch!

      // This part is confusing; if we're in the process of destroying this entity the chunk should be non-null
      // If we already destroyed the entity then the chunk will be null and then we continue?

      // If the chunk is non-null then the entity has been reused; or something like that? moved to a different chunk?

      if (chunk == nullptr) {
        continue; // if the chunk is null; it means that we've already destroyed this entity
      } else {
        break;
      }
    }
  }

  return { base_chunk, base_chunk_entity_index, chunk_entity_count };
}

void EntityManager::DestroyEntities(Entity* entities, i32 count) {
  u32*               versions       = version_by_entity_;
  _ChunkEntityIndex* chunk_indicies = entity_chunk_index_by_entity_;

  i32 i = 0;
  for (; i < count;) {
    _ChunkEntitySlice s = _FindFirstEntityRange(entities + i, count - i);

    Chunk*     chunk     = s.chunk_;
    Archetype* archetype = chunk->header_.archetype_;

    i32     free_index     = next_free_entity_index_;
    Entity* chunk_entities = chunk->EntityArray() + s.index_;

    // Why are we going backwards through the range?
    for (i32 i = s.count_ - 1; i >= 0; i--) {
      i32 entity_index = chunk_entities[i].index_;

      versions[entity_index]++;

      _ChunkEntityIndex* chunk_index = chunk_indicies + entity_index;
      chunk_index->chunk_            = nullptr;
      chunk_index->index_            = free_index;

      free_index = entity_index;
    }

    next_free_entity_index_ = free_index;
    entity_create_destroy_version_++;

    archetype->entity_count_ -= s.count_;
    i += s.count_;
  }
}

void EntityManager::_SetCapacity(i32 new_capacity) {
  auto old_capacity = entity_capacity_;
  if (new_capacity < old_capacity) {
    return; // We can never shrink since entity lookups can fail if we do
  }

  // Just the overhead from bookkeeping this many entities require gigabytes of memory
  assert(new_capacity < 128 * 1024 * 1024);

  version_by_entity_   = MemResizeArray(MEM_ALLOC_HEAP, version_by_entity_, old_capacity, new_capacity);
  archetype_by_entity_ = MemResizeArray(MEM_ALLOC_HEAP, archetype_by_entity_, old_capacity, new_capacity);
  entity_chunk_index_by_entity_ =
      MemResizeArray(MEM_ALLOC_HEAP, entity_chunk_index_by_entity_, old_capacity, new_capacity);

  // Initialize additional capacity

  for (i32 i = 0 < old_capacity ? old_capacity - 1 : 0; i < new_capacity; i++) {
    version_by_entity_[i] = 1;

    // While the chunk is null, store the index of the next entity to allocate
    // We do this because we're going to create holes later and when we do that
    // the next entity to allocate doesn't have to be continuous

    auto entity_in_chunk    = entity_chunk_index_by_entity_ + i;
    entity_in_chunk->chunk_ = nullptr;
    entity_in_chunk->index_ = i + 1;
  }

  auto last_entity_in_chunk    = &entity_chunk_index_by_entity_[new_capacity - 1];
  last_entity_in_chunk->index_ = -1; // Cork
}

EntityQuery* EntityManager::CreateQuery(const ComponentDataAccess* query_desc, i32 query_desc_len) {
  // Queries are pooled. We don't expect to find a lot of unique queries

  auto sorted = MemStackalloc(ComponentDataAccess, 0, query_desc_len);
  for (i32 i = 0; i < query_desc_len; i++) {
    sorted = Insert(sorted, query_desc[i]);
  }

  auto all             = MemStackalloc(ComponentTypeId, 0, query_desc_len);
  auto all_access_mode = MemStackalloc(ComponentDataAccess::Mode, 0, query_desc_len);

  auto any             = MemStackalloc(ComponentTypeId, 0, query_desc_len);
  auto any_access_mode = MemStackalloc(ComponentDataAccess::Mode, 0, query_desc_len);

  auto none             = MemStackalloc(ComponentTypeId, 0, query_desc_len);
  auto none_access_mode = MemStackalloc(ComponentDataAccess::Mode, 0, query_desc_len);

  for (auto component : sorted) {
    if (component.Exclude()) {
      none             = Append(none, component.type_id_);
      none_access_mode = Append(none_access_mode, component.access_mode_);
    } else if (component.Any()) {
      any             = Append(any, component.type_id_);
      any_access_mode = Append(any_access_mode, component.access_mode_);
    } else {
      all             = Append(all, component.type_id_);
      all_access_mode = Append(all_access_mode, component.access_mode_);
    }
  }

  EntityQuery tmp_query;
  MemZeroInit(&tmp_query);

  tmp_query.all_             = all.ptr_;
  tmp_query.all_access_mode_ = all_access_mode.ptr_;
  tmp_query.all_len_         = all.Len();

  tmp_query.any_             = any.ptr_;
  tmp_query.any_access_mode_ = any_access_mode.ptr_;
  tmp_query.any_len_         = any.Len();

  tmp_query.none_             = none.ptr_;
  tmp_query.none_access_mode_ = none_access_mode.ptr_;
  tmp_query.none_len_         = none.Len();

  u32 query_hash = tmp_query.HashCode();

  for (auto m : query_map_.Scan(query_hash)) {
    EntityQuery* existing_query = query_list_[m.Value()];
    if (existing_query->Equals(tmp_query)) {
      return existing_query;
    }
  }

  // ---
  // If we cannot find an existing query, we'll have to create a new query
  // ---

  EntityQuery* new_query = archetype_allocator_.Allocate<EntityQuery>();

  MemZeroInit(new_query);

  new_query->all_             = archetype_allocator_.AllocateArray<ComponentTypeId>(tmp_query.all_len_);
  new_query->all_access_mode_ = archetype_allocator_.AllocateArray<ComponentDataAccess::Mode>(tmp_query.all_len_);
  new_query->all_len_         = tmp_query.all_len_;

  MemCopyArray(new_query->all_, tmp_query.all_, tmp_query.all_len_);
  MemCopyArray(new_query->all_access_mode_, tmp_query.all_access_mode_, tmp_query.all_len_);

  new_query->any_             = archetype_allocator_.AllocateArray<ComponentTypeId>(tmp_query.any_len_);
  new_query->any_access_mode_ = archetype_allocator_.AllocateArray<ComponentDataAccess::Mode>(tmp_query.any_len_);
  new_query->any_len_         = tmp_query.any_len_;

  MemCopyArray(new_query->any_, tmp_query.any_, tmp_query.any_len_);
  MemCopyArray(new_query->any_access_mode_, tmp_query.any_access_mode_, tmp_query.any_len_);

  new_query->none_             = archetype_allocator_.AllocateArray<ComponentTypeId>(tmp_query.none_len_);
  new_query->none_access_mode_ = archetype_allocator_.AllocateArray<ComponentDataAccess::Mode>(tmp_query.none_len_);
  new_query->none_len_         = tmp_query.none_len_;

  MemCopyArray(new_query->none_, tmp_query.none_, tmp_query.none_len_);
  MemCopyArray(new_query->none_access_mode_, tmp_query.none_access_mode_, tmp_query.none_len_);

  assert(query_mask_count_ < EntityQueryMask::MAX_CAPACITY);
  new_query->mask_ = { uint8_t(query_mask_count_ / 8), uint8_t(query_mask_count_ % 8), this };
  query_mask_count_++;

  new_query->matching_archetypes_ = List<Archetype*>::WithAllocator(MEM_ALLOC_HEAP);

  for (auto archetype : archetypes_.list_) {
    if (new_query->IsMatch(archetype)) {
      new_query->matching_archetypes_.Add(archetype);
      archetype->matching_queries_.Add(new_query);
    }
  }

  query_map_.Add(query_hash, query_list_.Len());
  query_list_.Add(new_query);

  return new_query;
}