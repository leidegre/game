#pragma once

#include "../ecs/ecs.hh"

namespace game {
struct Archetype;

struct GameState {
  World world_;
  struct {
    Archetype* entity_position_;
  } archetypes_;
  struct {
    bool show_archetypes_window_;
    bool show_entities_window_;
  } ui_;
};

void GameInit(GameState* state);

void GameUpdate(GameState* state);

void GameRender(GameState* state);

void GameShutdown(GameState* state);
} // namespace game