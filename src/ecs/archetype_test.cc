#include "../test/test.h"

#include "archetype.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("ArchetypeChunkDataTest") {
    ArchetypeChunkData data;

    data.Create(1, 2040);

    Chunk chunk1;
    MemZeroInit(&chunk1);

    data.Add(&chunk1, 1);

    ASSERT_EQUAL_PTR(&chunk1, data.ChunkPtrArray()[0]);
    ASSERT_EQUAL_U32(1, data.ChangeVersionArray(0)[0]);
    ASSERT_EQUAL_U32(0, data.EntityCountArray()[0]);

    Chunk chunk2;
    MemZeroInit(&chunk2);

    data.Add(&chunk2, 2);

    ASSERT_EQUAL_PTR(&chunk1, data.ChunkPtrArray()[0]);
    ASSERT_EQUAL_U32(1, data.ChangeVersionArray(0)[0]);
    ASSERT_EQUAL_U32(0, data.EntityCountArray()[0]);

    ASSERT_EQUAL_PTR(&chunk2, data.ChunkPtrArray()[1]);
    ASSERT_EQUAL_U32(2, data.ChangeVersionArray(0)[1]);
    ASSERT_EQUAL_U32(0, data.EntityCountArray()[1]);

    data.Destroy();
  }
}
