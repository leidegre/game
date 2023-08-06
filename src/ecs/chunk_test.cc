#include "../test/test.h"

#include "chunk.hh"

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("ChunkTest") {
    ASSERT_EQUAL_SIZE(64, sizeof(ChunkHeader));
    ASSERT_EQUAL_SIZE(64, alignof(ChunkHeader));

    ASSERT_EQUAL_SIZE(128, sizeof(Chunk));
    ASSERT_EQUAL_SIZE(64, alignof(Chunk));

    Chunk chunk;

    ASSERT_EQUAL_U64(64, (ptrdiff_t)chunk.Buffer() - (ptrdiff_t)&chunk);
  }

  TEST_CASE("ChunkAllocatorTest") {
    auto allocator = MemAllocZeroInit<ChunkAllocator>(MEM_ALLOC_HEAP);

    ASSERT_FALSE(allocator->mega_chunks_[0]);

    auto chunk0 = allocator->Allocate();
    ASSERT_TRUE(chunk0 != nullptr);
    ASSERT_EQUAL_U64(0x8000000000000000ULL, allocator->chunk_use_[0]);
    ASSERT_TRUE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    auto chunk1 = allocator->Allocate();
    ASSERT_TRUE(chunk1 != nullptr);
    ASSERT_EQUAL_U64(0xC000000000000000ULL, allocator->chunk_use_[0]);
    ASSERT_TRUE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    auto chunk2 = allocator->Allocate();
    ASSERT_TRUE(chunk2 != nullptr);
    ASSERT_EQUAL_U64(0xE000000000000000ULL, allocator->chunk_use_[0]);
    ASSERT_TRUE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    allocator->Free(chunk1);
    ASSERT_EQUAL_U64(0xA000000000000000ULL, allocator->chunk_use_[0]);
    ASSERT_TRUE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    auto chunk3 = allocator->Allocate();
    ASSERT_TRUE(chunk3 != nullptr);
    ASSERT_EQUAL_U64(0xB000000000000000ULL, allocator->chunk_use_[0]);
    ASSERT_TRUE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    allocator->Free(chunk0);
    allocator->Free(chunk2);
    allocator->Free(chunk3);

    ASSERT_EQUAL_U64(0, allocator->chunk_use_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[0]);
    ASSERT_FALSE(allocator->mega_chunks_[1]);

    MemFree(MEM_ALLOC_HEAP, allocator);
  }
}
