#include "world.hh"

using namespace game;

void World::Create(Slice<const TypeInfo> components) {
  MemZeroInit(this);

  type_registry_ = MemAllocZeroInit<ComponentRegistry>(MEM_ALLOC_HEAP);
  type_registry_->Initialize(components);

  chunk_allocator_ = MemAllocZeroInit<ChunkAllocator>(MEM_ALLOC_HEAP);

  entity_manager_ = MemAllocZeroInit<game::EntityManager>(MEM_ALLOC_HEAP);
  entity_manager_->Create(this, 1024);

  system_list_  = List<System*>::WithAllocator(MEM_ALLOC_HEAP);
  system_state_ = List<SystemState>::WithAllocator(MEM_ALLOC_HEAP);
}

void World::Destroy() {
  for (int i = 0; i < system_list_.Len(); i++) {
    System*      system = system_list_[i];
    SystemState& state  = system_state_[i];

    if ((state.flags_ & (SystemState::FLAG_CREATED | SystemState::FLAG_DESTROYED)) == SystemState::FLAG_CREATED) {
      system->OnDestroy(state);
      state.flags_ |= SystemState::FLAG_DESTROYED;
    }
  }

  system_list_.Destroy();
  system_state_.Destroy();

  entity_manager_->Destroy();
  chunk_allocator_->Destroy();

  MemFree(MEM_ALLOC_HEAP, entity_manager_);
  MemFree(MEM_ALLOC_HEAP, chunk_allocator_);
  MemFree(MEM_ALLOC_HEAP, type_registry_);
}

void World::Update() {
  for (int i = 0; i < system_list_.Len(); i++) {
    System*      system = system_list_[i];
    SystemState& state  = system_state_[i];

    // If the system is not created and not destroyed we will create it
    if ((state.flags_ & (SystemState::FLAG_CREATED | SystemState::FLAG_DESTROYED)) == 0) {
      system->OnCreate(state);
      state.flags_ |= SystemState::FLAG_CREATED | SystemState::FLAG_RUNNING;
    }

    // If the system is not paused we will update it
    if ((state.flags_ & (SystemState::FLAG_RUNNING)) == SystemState::FLAG_RUNNING) {
      system->OnUpdate(state);
    }
  }
}