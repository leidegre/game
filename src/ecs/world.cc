#include "chunk.hh"
#include "entity-manager.hh"

using namespace game;

void World::Create(Slice<const TypeInfo> components) {
  MemZeroInit(this);

  type_registry_ = MemAllocZeroInit<ComponentRegistry>(MEM_ALLOC_HEAP);
  type_registry_->Initialize(components);

  chunk_allocator_ = MemAllocZeroInit<ChunkAllocator>(MEM_ALLOC_HEAP);

  entity_manager_ = MemAllocZeroInit<game::EntityManager>(MEM_ALLOC_HEAP);
  entity_manager_->Create(this, 1024);
}

void World::Destroy() {
  entity_manager_->Destroy();
  chunk_allocator_->Destroy();

  MemFree(MEM_ALLOC_HEAP, entity_manager_);
  MemFree(MEM_ALLOC_HEAP, chunk_allocator_);
  MemFree(MEM_ALLOC_HEAP, type_registry_);
}