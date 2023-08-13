#include "../test/test.h"

#include "local-to-world-system.hh"

#include "world.hh"

#include "../components/components.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("SystemTest") {
    World world;

    world.Create(GetComponentTypeInfoArray());

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
