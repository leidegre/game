#include "archetype.hh"

using namespace game;

void ArchetypeChunkData::Create(i32 component_count, i32 chunk_entity_capacity) {
  MemZeroInit(this);

  component_count_       = component_count;
  chunk_entity_capacity_ = chunk_entity_capacity;
}

void ArchetypeChunkData::Destroy() {
  if (ptr_ != nullptr) {
    MemFree(MEM_ALLOC_HEAP, ptr_);
    ptr_ = nullptr;
  }
  len_                   = 0;
  cap_                   = 0;
  component_count_       = 0;
  chunk_entity_capacity_ = 0;
}

void ArchetypeChunkData::Add(Chunk* chunk, u32 change_version) {
  if (!(len_ < cap_)) {
    auto temp = *this;

    cap_ = cap_ == 0 ? 1 : 2 * cap_;
    ptr_ = MemAlloc(MEM_ALLOC_HEAP, _BufferSize(), 16);

    MemCopy(ChunkPtrArray(), temp.ChunkPtrArray(), temp._ChunkPtrArraySize());
    MemCopy(ChangeVersionArray(), temp.ChangeVersionArray(), temp._ChangeVersionArraySize());
    MemCopy(EntityCountArray(), temp.EntityCountArray(), temp._EntityCountArraySize());

    temp.Destroy();

    assert(len_ < cap_);
  }

  const int chunk_index        = len_++;
  chunk->header_.list_index_   = chunk_index;
  ChunkPtrArray()[chunk_index] = chunk;

  // the change versions are stored like a row-major matrix where each row is the change versions for a component type
  // with all the change versions for component type at index 0 (in the archetype) contiguously in memory

  // type[0]: chunk[0] chunk[1] chunk[2]
  // type[1]: chunk[0] chunk[1] chunk[2]
  // type[2]: chunk[0] chunk[1] chunk[2]

  for (int i = 0; i < component_count_; i++) {
    ChangeVersionArray(i)[chunk_index] = change_version;
  }

  EntityCountArray()[chunk_index] = chunk->EntityCount();
}

Archetype* ArchetypeListMap::TryGet(Slice<const ComponentTypeId> types) {
  u32 types_hash = HashData(types);
  for (auto entry : map_.Scan(types_hash)) {
    i32        i         = entry.Value(); // index of archetype in list
    Archetype* archetype = list_[i];
    if (Equals(archetype->TypeId(), types)) {
      return archetype;
    }
  }
  return nullptr;
}

void ArchetypeListMap::Add(Archetype* archetype) {
  u32  types_hash = HashData(archetype->TypeId());
  bool added      = map_.Add(types_hash, list_.Len());
  list_.Add(archetype);
  assert(added && "archetype was not added (did you forget to initialize hash map?)");
}