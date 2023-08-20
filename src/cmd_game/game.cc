#include "game.hh"

#include <imgui.h>

using namespace game;

struct Position {
  enum { COMPONENT_TYPE = 1 };

  float v_[3];
};

struct Rotation {
  enum { COMPONENT_TYPE = 2 };

  float axis_[3];
  float angle_;
};

static const TypeInfo components[] = {
  GAME_COMPONENT(Entity),
  GAME_COMPONENT(Position),
  GAME_COMPONENT(Rotation),
};

void game::GameInit(GameState* state) {
  MemZeroInit(state);

  state->world_.Create(slice::FromArray(components));

  state->world_.EntityManager().CreateEntity();
  state->world_.EntityManager().CreateEntity();
  state->world_.EntityManager().CreateEntity();

  const ComponentTypeId types[] = {
    GetComponentTypeId<Position>(),
  };

  state->archetypes_.entity_position_         = state->world_.entity_manager_->CreateArchetype(slice::FromArray(types));
  state->archetypes_.entity_position_->label_ = "Entity+Position";

  state->world_.entity_manager_->CreateEntities(state->archetypes_.entity_position_, nullptr, 1);
}

void game::GameUpdate(GameState* state) {
  // at the moment... does nothing
}

// ---

void ShowEntityWindow(GameState* state) {
  if (!ImGui::Begin("Entities", &state->ui_.show_entities_window_)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTable("table 5", 5)) {
    ImGui::TableSetupColumn("Id");
    ImGui::TableSetupColumn("Version");
    ImGui::TableSetupColumn("Chunk");
    ImGui::TableSetupColumn("Index (in chunk)");
    ImGui::TableSetupColumn("Archetype");
    ImGui::TableHeadersRow();

    // We don't have a straight forward way of enumerating all entities because we don't have an entity count in the entity manager... I find that peculiar

    for (auto archetype : state->world_.entity_manager_->archetypes_.list_) {
      for (i32 chunk_index = 0; chunk_index < archetype->chunk_data_.Len(); chunk_index++) {
        auto chunk    = archetype->chunk_data_.ChunkPtrArray()[chunk_index];
        auto entities = chunk->EntityArray();
        for (i32 i = 0; i < chunk->EntityCount(); i++) {
          auto entity = entities + i;

          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%i", entity->index_);

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%i", entity->version_);

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%p", chunk);

          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%i", i);

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%s", archetype->label_);
        }
      }
    }

    ImGui::EndTable();
  }

  ImGui::End();
}

void ShowEntityArchetypeWindow(GameState* state) {
  if (!ImGui::Begin("Archetypes", &state->ui_.show_archetypes_window_)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTable("Archetype Table", 3)) {
    ImGui::TableSetupColumn("Label");
    ImGui::TableSetupColumn("Entities");
    ImGui::TableSetupColumn("Chunks (in use/free)");
    ImGui::TableHeadersRow();

    for (auto archetype : state->world_.entity_manager_->archetypes_.list_) {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", archetype->label_);

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%i", archetype->entity_count_);

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%i/%i", archetype->chunk_data_.Len(), archetype->chunk_with_empty_slots_.Len());
    }

    ImGui::EndTable();
  }

  ImGui::End();
}

void game::GameRender(GameState* state) {
  static bool show_demo_window = false;

  ImGuiIO* io = &ImGui::GetIO();

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Game")) {
      if (ImGui::MenuItem("Exit")) {
        // todo: ...
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entities")) {
      ImGui::MenuItem("Archetypes", nullptr, &state->ui_.show_archetypes_window_);
      ImGui::MenuItem("Entities", nullptr, &state->ui_.show_entities_window_);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("ImGui")) {
      ImGui::MenuItem("Show Demo Window", nullptr, &show_demo_window);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }

  if (state->ui_.show_archetypes_window_) {
    ShowEntityArchetypeWindow(state);
  }

  if (state->ui_.show_entities_window_) {
    ShowEntityWindow(state);
  }
}

void game::GameShutdown(GameState* state) {
  state->world_.Destroy();
}