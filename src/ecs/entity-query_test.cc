#include "../test/test.h"

#include "archetype.hh"
#include "component-registry.hh"
#include "entity-manager.hh"
#include "entity-query.hh"

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

struct LocalToWorld {
  enum { COMPONENT_TYPE = 3 };

  float m_[16];
};
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  const TypeInfo components[] = {
    GAME_COMPONENT(Entity),
    GAME_COMPONENT(Position),
    GAME_COMPONENT(Rotation),
    GAME_COMPONENT(LocalToWorld),
  };

  TEST_CASE("CreateEntityQueryTest") {
    World world;

    world.Create(slice::FromArray(components));

    Archetype* archetype1 = world.EntityManager().CreateArchetype({ GetComponentTypeId<Position>() });
    Archetype* archetype2 = world.EntityManager().CreateArchetype({ GetComponentTypeId<Rotation>() });

    Entity a = world.EntityManager().CreateEntity(archetype1);
    Entity b = world.EntityManager().CreateEntity(archetype2);

    EntityQuery* q1 = world.EntityManager().CreateQuery({ ComponentDataAccess::Read<Position>() });

    ASSERT_EQUAL_I32(1, world.EntityManager().query_list_.Len());

    ASSERT_EQUAL_I32(1, q1->matching_archetypes_.Len());
    ASSERT_EQUAL_PTR(archetype1, q1->matching_archetypes_[0]);

    EntityQuery* q2 = world.EntityManager().CreateQuery({ ComponentDataAccess::Read<Rotation>() });

    ASSERT_EQUAL_I32(2, world.EntityManager().query_list_.Len());

    ASSERT_EQUAL_I32(1, q1->matching_archetypes_.Len());
    ASSERT_EQUAL_PTR(archetype1, q1->matching_archetypes_[0]);

    ASSERT_EQUAL_I32(1, q2->matching_archetypes_.Len());
    ASSERT_EQUAL_PTR(archetype2, q2->matching_archetypes_[0]);

    world.Destroy();
  }
}
