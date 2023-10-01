#pragma once

#include "../common/type-system.hh"

namespace game {
// pointer to implementation of rendering system
struct Renderer;

// Initialize the rendering system
bool RenderInit(Renderer** render);

// Since the renderer owns the window it is also responsible for processing the message loop
// If the return value for this function is false the game should quit
bool RenderUpdateInput(Renderer& render);

// Begin default render pass
bool RenderFrameBegin(Renderer& render);

// End default render pass
bool RenderFrameEnd(Renderer& render);

// Use RenderFrameBegin/RenderFrameEnd instead
bool RenderUpdateFrame(Renderer& render);

// Insert a fence into command queue, use RenderWaitFor... to wait for the last fence for this frame
u64 RenderFence(Renderer& render);

// Wait for current frame to complete.
bool RenderWaitForCurr(Renderer& render);

// Wait for previous frame to complete.
bool RenderWaitForPrev(Renderer& render);

// Cleanup rendering system
bool RenderShutdown(Renderer& render);

uint32_t RenderDebugInfoQueueMessageCount(Renderer& render);
} // namespace game