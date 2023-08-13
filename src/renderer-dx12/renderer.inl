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

// https://developer.nvidia.com/dx12-dos-and-donts

namespace game {
enum {
  RENDERER_BACK_BUFFER_COUNT = 2,
};

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
};

LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
} // namespace game