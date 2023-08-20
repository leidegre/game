#include "tex-loader.hh"

#include "../test/test.h"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("LoadTextureFromFile(*.png)") {
    Error        err;
    TextureAsset tex;
    ASSERT_TRUE((err = LoadTextureFromFile(MEM_ALLOC_HEAP, "data/textures/debug/xz-grid-1024.png", &tex)).Ok());
  }

  return 0;
}