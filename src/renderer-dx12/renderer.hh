#pragma once

namespace game {
// pointer to implementation of rendering system
struct Renderer;

// Initialize the rendering system
bool RenderInit(Renderer** render);

// Since the renderer owns the window it is also responsible for processing the message loop
// If the return value for this function is false the game should quit
bool RenderUpdateInput(Renderer& render);

// Draw frame
bool RenderUpdateFrame(Renderer& render);

// Cleanup rendering system
bool RenderShutdown(Renderer& render);
} // namespace game