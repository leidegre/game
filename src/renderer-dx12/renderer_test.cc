#include "renderer.hh"

#include <cstdio>

using namespace game;

int main(int, char**) {
  Renderer* r;
  if (!RenderInit(&r)) {
    puts("cannot initialize renderer");
    return 1;
  }
  for (int i = 0; i < 144; i++) {
    if (!RenderUpdateInput(*r)) {
      puts("cannot update input");
      return 1;
    }
    if (!RenderUpdateFrame(*r)) {
      puts("cannot update frame");
      return 1;
    }
  }
  if (!RenderShutdown(*r)) {
    puts("cannot shutdown renderer");
    return 1;
  }
  return 0;
}
