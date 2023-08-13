#include "../test/test.h"

#include "archetype.hh"
#include "component-registry.hh"
#include "entity-manager.hh"

using namespace game;

// todo: need to use component flags to signal that this component is a zero size component

namespace {
struct TagComponent {
  enum { COMPONENT_TYPE = 1 };
};
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  const TypeInfo components[] = {
    GAME_COMPONENT(Entity),
    GAME_COMPONENT(TagComponent),
  };

  TEST_CASE("TagComponentTest") {
    ASSERT_EQUAL_SIZE(1, sizeof(TagComponent));
  }
}
