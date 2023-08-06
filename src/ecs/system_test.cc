#include "../test/test.h"

#include "archetype.hh"
#include "component-registry.hh"
#include "entity-manager.hh"
#include "entity-query.hh"
#include "system.hh"

#include "../math/transform.hh"

using namespace game;

#include "components_test.inl"

namespace {
template <typename T> struct ComponentHandle {
  ComponentTypeId type_id_;
};

template <typename T> ComponentHandle<T> GetComponentHandle() {
  return { GetComponentTypeId<T>() };
}

// Not really a chunk, more like a slice of a chunk
struct ArchetypeChunk {
  Chunk* chunk_;
  int    batch_begin_index_;
  int    batch_end_index_;

  int Len() const {
    return batch_end_index_ - batch_begin_index_;
  }

  template <typename T> T* GetArray(const ComponentHandle<T>& component) const {
    // static_assert(!GetComponentTypeId<T>().IsTagComponent(), "cannot be used with zero sized component");

    Archetype*       archetype            = chunk_->Archetype();
    ComponentTypeId* archetype_types      = archetype->types_;
    i32              archetype_types_len_ = archetype->types_len_;

    // index of component type in archetype (less than 100 is free)
    int i = 0;
    for (; i < archetype_types_len_; i++) {
      if (archetype_types[i] == component.type_id_) {
        break;
      }
    }
    if (!(i < archetype_types_len_)) {
      return nullptr; // when using any queries, it is possible to not have any data for a particular component type
    }

    i32 offset = archetype->offsets_[i];
    i32 size   = archetype->sizes_[i];

    // if this is a write we unconditionally set the global system version of the type in the chunk
    // archetype->chunk_data_.ChangeVersionsPtr()[i] = component.global_system_version_;

    auto ptr1 = (byte*)chunk_->Buffer() + offset + size * i;
    auto ptr2 = ptr1 + size * batch_begin_index_;

    return (T*)ptr2;
  }
};

template <typename T>
void ExecuteJob(EntityQuery* query, T& job_data, void (*job_kernel)(T& data, const ArchetypeChunk& chunk)) {
  for (auto archetype : query->matching_archetypes_) {
    auto chunk_data = &archetype->chunk_data_;
    for (int i = 0; i < chunk_data->Len(); i++) {
      auto           chunk           = chunk_data->ChunkPtrArray()[i];
      ArchetypeChunk archetype_chunk = { chunk, 0, chunk->EntityCount() };
      job_kernel(job_data, archetype_chunk);
    }
  }
}

struct TRS_LocalToWorldJobData {
  ComponentHandle<Translation>  translation_handle_;
  ComponentHandle<Rotation>     rotation_handle_;
  ComponentHandle<Scale>        scale_handle_;
  ComponentHandle<LocalToWorld> local_to_world_handle_;
};

void TRS_LocalToWorldJobKernel(TRS_LocalToWorldJobData& data, const ArchetypeChunk& chunk) {
  Translation*  translation    = chunk.GetArray(data.translation_handle_); // optional
  Rotation*     rotation       = chunk.GetArray(data.rotation_handle_);    // optional
  Scale*        scale          = chunk.GetArray(data.scale_handle_);       // optional
  LocalToWorld* local_to_world = chunk.GetArray(data.local_to_world_handle_);

  if (translation != nullptr) {
    if (rotation != nullptr) {
      if (scale != nullptr) {
        // TRS
      } else {
        // TR
      }
    } else {
      // Without rotation
      if (scale != nullptr) {
        // TS
      } else {
        // T
      }
    }
  } else {
    // Without translation
    if (rotation != nullptr) {
      if (scale != nullptr) {
        // RS
      } else {
        // R
      }
    } else {
      // Without rotation
      if (scale != nullptr) {
        // S
      } else {
        // Identity
        for (i32 i = 0; i < chunk.Len(); i++) {
          local_to_world[i].value_ = mat4::Identity();
        }
      }
    }
  }
}

struct TRS_LocalToWorldSystem : public System {
  EntityQuery* q_;

  void OnCreate(SystemState& state) override {
    q_ = state.EntityManager().CreateQuery({ ComponentDataAccess::Write<LocalToWorld>(),
                                             ComponentDataAccess::ReadAny<Translation>(),
                                             ComponentDataAccess::ReadAny<Rotation>(),
                                             ComponentDataAccess::ReadAny<Scale>() });
  }

  void OnUpdate(SystemState& state) override {
    TRS_LocalToWorldJobData data;
    data.translation_handle_    = GetComponentHandle<Translation>();  // read
    data.rotation_handle_       = GetComponentHandle<Rotation>();     // read
    data.scale_handle_          = GetComponentHandle<Scale>();        // read
    data.local_to_world_handle_ = GetComponentHandle<LocalToWorld>(); // write
    ExecuteJob(q_, data, TRS_LocalToWorldJobKernel);
  }
};
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  const TypeInfo components[] = { GAME_COMPONENT(Entity),
                                  GAME_COMPONENT(Translation),
                                  GAME_COMPONENT(Rotation),
                                  GAME_COMPONENT(Scale),
                                  GAME_COMPONENT(LocalToWorld) };

  TEST_CASE("SystemTest") {
    World world;

    world.Create(slice::FromArray(components));

    Archetype* t = world.EntityManager().CreateArchetype({
        GetComponentTypeId<LocalToWorld>(),
        GetComponentTypeId<Translation>(),
    });
    Archetype* r = world.EntityManager().CreateArchetype({
        GetComponentTypeId<LocalToWorld>(),
        GetComponentTypeId<Rotation>(),
    });
    Archetype* s = world.EntityManager().CreateArchetype({
        GetComponentTypeId<LocalToWorld>(),
        GetComponentTypeId<Scale>(),
    });

    world.EntityManager().CreateEntity(t);
    world.EntityManager().CreateEntity(r);
    world.EntityManager().CreateEntity(s);

    SystemState state;
    MemZeroInit(&state);
    state.entity_manger_ = world.entity_manager_;

    TRS_LocalToWorldSystem transform_system;
    MemZeroInit(&transform_system);

    transform_system.OnCreate(state);
    transform_system.OnUpdate(state);
    transform_system.OnDestroy(state);

    world.Destroy();
  }
}
