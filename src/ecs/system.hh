#pragma once

#include "entity-manager.hh"

namespace game {
struct EntityManager;

// ---

// Represents read only access to component data array
template <typename T> struct ComponentDataReader {
  ComponentTypeId type_id_;
  ComponentDataReader() { type_id_ = GetComponentTypeId<T>(); }
};

// Represents read & write access to component data array
template <typename T> struct ComponentDataReaderWriter {
  ComponentTypeId type_id_;
  ComponentDataReaderWriter() { type_id_ = GetComponentTypeId<T>(); }
};

// A slice of a chunk for processing in a job kernel. ChunkDataView?
struct SystemChunk {
  Chunk* chunk_;
  int    batch_begin_index_;
  int    batch_end_index_;

  int Len() const { return batch_end_index_ - batch_begin_index_; }

  void* _GetArray(ComponentTypeId component_type_id) const {
    Archetype&       archetype            = chunk_->Archetype();
    ComponentTypeId* archetype_types      = archetype.types_;
    i32              archetype_types_len_ = archetype.types_len_;

    // Find index of component type in archetype
    int i = 0;
    for (; i < archetype_types_len_; i++) {
      if (archetype_types[i] == component_type_id) {
        break;
      }
    }
    if (!(i < archetype_types_len_)) {
      return nullptr; // when using any queries, it is possible to not have any data for a particular component type
    }

    i32 offset = archetype.offsets_[i];
    i32 size   = archetype.sizes_[i];

    // if this is a write we unconditionally set the global system version of the type in the chunk
    // archetype->chunk_data_.ChangeVersionsPtr()[i] = component.global_system_version_;

    auto ptr1 = (byte*)chunk_->Buffer() + offset + size * i;
    auto ptr2 = ptr1 + size * batch_begin_index_;

    return ptr2;
  }

  template <typename T> const T* GetArray(const ComponentDataReader<T>& reader) const {
    // todo: static assert for zero sized components
    return (const T*)_GetArray(reader.type_id_);
  }

  template <typename T> T* GetArray(const ComponentDataReaderWriter<T>& reader) const {
    // todo: static assert for zero sized components
    return (T*)_GetArray(reader.type_id_);
  }
};

// ---

// put the system state in the base class?
struct SystemState {
  enum Flags : u32 {
    FLAG_CREATED   = 1 << 0,
    FLAG_RUNNING   = 1 << 1,
    FLAG_DESTROYED = 1 << 2,
  };

  EntityManager* entity_manger_;
  EntityManager& EntityManager() {
    assert(entity_manger_);
    return *entity_manger_;
  }

  u32 flags_;

  f32 dT_; // delta time in seconds since last frame
};

struct System {
  virtual void OnCreate(SystemState& state) {
    //...
  }
  virtual void OnUpdate(SystemState& state) {
    //...
  }
  virtual void OnDestroy(SystemState& state) {
    //...
  }

  // utilities for computation

  template <typename T>
  static void ExecuteJob(EntityQuery* query, T& job_data, void (*job_kernel)(T& data, const SystemChunk& chunk)) {
    for (auto archetype : query->matching_archetypes_) {
      auto chunk_data = &archetype->chunk_data_;
      for (int i = 0; i < chunk_data->Len(); i++) {
        Chunk*      chunk           = chunk_data->ChunkPtrArray()[i];
        SystemChunk archetype_chunk = { chunk, 0, chunk->EntityCount() };
        job_kernel(job_data, archetype_chunk);
      }
    }
  }
};
} // namespace game
