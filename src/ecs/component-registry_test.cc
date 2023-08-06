#include "../test/test.h"

#include "component-registry.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("TypeManagerCreateTest") {
    ComponentRegistry component_registry;

    struct Position {
      enum { COMPONENT_TYPE = 1 };

      f32 pos_[3]; // xyz
    };

    // the life time this array needs to match the life time of the component registry
    static const TypeInfo type_info[] = {
      GAME_COMPONENT(Entity), // the first component must always be the Entity component
      GAME_COMPONENT(Position),
    };

    component_registry.Initialize(slice::FromArray(type_info));
  }
}
