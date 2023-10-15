// Vertex Data Viewer

#include "../renderer-dx12/renderer.hh"

#include "../common/cli.hh"

using namespace game;

int main(int argc, char** argv) {
  const char* open = nullptr;

  CommandLineInterface cli{};
  cli.AddStr(&open, "open");
  cli.MustParse(argc, argv); // exit on error

  Renderer* r;
  RenderInit(&r);

  // todo: read VDF file and render with a flat shader

  for (; RenderUpdateInput(*r);) {
    RenderFrameBegin(*r);
    RenderFrameEnd(*r);
  }

  RenderWaitForPrev(*r);
  RenderShutdown(*r);
  return 0;
}