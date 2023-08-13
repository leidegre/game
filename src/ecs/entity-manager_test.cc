#include "../test/test.h"

#include "entity-manager.hh"

#include "world.hh"

using namespace game;

namespace {
struct Position {
  enum { COMPONENT_TYPE = 1 };

  float v_[3];
};

struct Rotation {
  enum { COMPONENT_TYPE = 2 };

  float axis_[3];
  float angle_;
};
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  const TypeInfo components[] = {
    GAME_COMPONENT(Entity),
    GAME_COMPONENT(Position),
    GAME_COMPONENT(Rotation),
  };

  TEST_CASE("CreateArchetypeTest") {
    World world;

    world.Create(slice::FromArray(components));

    auto archetype = world.entity_manager_->CreateArchetype({
        GetComponentTypeId<Rotation>(), // unsorted
        GetComponentTypeId<Position>(),
    });

    ASSERT_EQUAL_I32(archetype->types_len_, 3); // Entity + Position + Rotation

    ASSERT_EQUAL_I32(archetype->types_[0].v_, GetComponentTypeId<Entity>().v_);
    ASSERT_EQUAL_I32(archetype->types_[1].v_, GetComponentTypeId<Position>().v_);
    ASSERT_EQUAL_I32(archetype->types_[2].v_, GetComponentTypeId<Rotation>().v_);

    world.Destroy();
  }

  TEST_CASE("CreateEntityTest") {
    World world;

    world.Create(slice::FromArray(components));

    const ComponentTypeId types[] = {
      GetComponentTypeId<Position>(),
      GetComponentTypeId<Rotation>(),
    };

    auto archetype = world.entity_manager_->CreateArchetype(slice::FromArray(types));

    Entity entities[1];

    world.entity_manager_->CreateEntity(archetype, entities, 1);

    auto chunk = archetype->chunk_data_.ChunkPtrArray()[0];

    auto entity_chunk_index = world.entity_manager_->entity_chunk_index_by_entity_[0];

    ASSERT_EQUAL_PTR(chunk, entity_chunk_index.chunk_);
    ASSERT_EQUAL_U32(0, entity_chunk_index.index_);

    world.Destroy();
  }

  TEST_CASE("CreateDestroyEntityTest") {
    World world;

    world.Create(slice::FromArray(components));

    Entity a = world.EntityManager().CreateEntity();
    Entity b = world.EntityManager().CreateEntity();
    Entity c = world.EntityManager().CreateEntity();

    // Entities are allocated in ascending order

    ASSERT_EQUAL_I32(0, a.index_);
    ASSERT_EQUAL_U32(1, a.version_); // initially 1

    ASSERT_EQUAL_I32(1, b.index_);
    ASSERT_EQUAL_U32(1, b.version_);

    ASSERT_EQUAL_I32(2, c.index_);
    ASSERT_EQUAL_U32(1, c.version_);

    ASSERT_EQUAL_I32(3, world.EntityManager().next_free_entity_index_);

    world.EntityManager().DestroyEntity(b);

    // When an entity is destroyed in the middle of a chunk
    // it's place will become available for reuse but no effort
    // is made to compact the chunk

    ASSERT_EQUAL_I32(1, world.EntityManager().next_free_entity_index_);

    Entity d = world.EntityManager().CreateEntity();

    ASSERT_EQUAL_I32(1, d.index_);
    ASSERT_EQUAL_U32(2, d.version_); // this has now been reused once

    ASSERT_EQUAL_I32(3, world.EntityManager().next_free_entity_index_);

    world.Destroy();
  }
}
