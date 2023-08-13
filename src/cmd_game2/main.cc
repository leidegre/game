#include "../components/components.hh"
#include "../ecs/ecs.hh"
#include "../ecs/local-to-world-system.hh"
#include "../renderer-dx12/renderer.hh"

using namespace game;

int main(int, char**) {
  World w;
  w.Create(GetComponentTypeInfoArray());

  // Systems have different latency requirements
  // and they don't need to run sequentially in lockstep
  // For now we're doing everything in a serial fashion

  TRS_LocalToWorldSystem local_to_world_system{};
  w.Register(&local_to_world_system);

  Renderer* r;
  RenderInit(&r);
  for (; RenderUpdateInput(*r);) {
    w.Update();
    RenderUpdateFrame(*r);
  }
  RenderShutdown(*r);

  w.Destroy();
  return 0;
}
