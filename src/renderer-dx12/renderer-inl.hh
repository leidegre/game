#pragma once

#include <d3d12.h>
#include <d3dx12.h> // helpers

#include <dxgi1_4.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#endif

#include <d3dcompiler.h>

#include "undo-windows-h-shenanigans.hh"

#include "../math/transform.hh"

// https://developer.nvidia.com/dx12-dos-and-donts

namespace game {
enum {
  RENDERER_BACK_BUFFER_COUNT = 2,
};

// Maybe we should draw some inspiration from this example?
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/TechniqueDemos/D3D12MemoryManagement/src/Framework.cpp#L689

struct Renderer {
  HWND wnd_;

#ifdef DX12_ENABLE_DEBUG_LAYER
  ID3D12Debug* debug_layer_;
#endif

  ID3D12Device*               dev_;
  ID3D12DescriptorHeap*       rtv_desc_heap_;
  D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle_[RENDERER_BACK_BUFFER_COUNT];
  ID3D12Resource*             rtv_buffer_[RENDERER_BACK_BUFFER_COUNT];
  ID3D12DescriptorHeap*       cbv_desc_heap_; // can be used for CBV, SRV or UAV data
  ID3D12CommandQueue*         cmd_queue_;
  ID3D12CommandAllocator*     cmd_allocator_[RENDERER_BACK_BUFFER_COUNT];
  ID3D12GraphicsCommandList*  cmd_list_;
  ID3D12Fence*                fence_;
  HANDLE                      fence_event_; // auto reset Win32 event, maybe not share this with every frame...
  UINT64                      fence_source_;
  UINT64                      fence_value_[RENDERER_BACK_BUFFER_COUNT];
  UINT64                      NextFenceValue() { return ++fence_source_; }

  UINT32 frame_number_;

  IDXGISwapChain3* swap_chain_; // GetCurrentBackBufferIndex

  // signalled when when the number of frames waiting to be presented drops below the maximum frame latency
  HANDLE swap_chain_waitable_obj_;

  vec3 view_eye_;    // Camera position
  vec3 view_target_; // Camera target
  vec3 view_up_;     // Camera up
  mat4 view_;        // View matrix

  f32  proj_fovy_;
  f32  proj_aspect_;
  f32  proj_near_;
  f32  proj_far_;
  mat4 proj_; // Projection matrix

  mat4 view_proj_; // View+Projection matrix

  ID3DBlob*            grid_vs_;
  ID3DBlob*            grid_ps_;
  ID3D12RootSignature* grid_root_signature_;
  ID3D12PipelineState* grid_pso_;
  ID3D12Resource*      grid_xz_tex_;
  ID3D12Resource*      grid_xz_tex_upload_;
  ID3D12Resource*      grid_xy_tex_;

  UINT32 dxgi_info_queue_message_count_;
};

LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
} // namespace game