#include "renderer.hh"
#include "renderer.inl"

#include "../common/mem.hh"

using namespace game;

// ---
// logger code, let's move that somewhere else

enum LogLevel {
  GAME_LOG_ERROR,
};

enum LogCategory {
  GAME_LOG_CATEGORY_DX12, // scope
};

struct Logger {
  LogCategory category_;

  void LogError(const char* message) { fprintf(stderr, "%s\n", message); }
  void LogWarning(const char* message) { fprintf(stderr, "%s\n", message); }
};

// ---

Logger g_dx12_log = { GAME_LOG_CATEGORY_DX12 };

bool game::RenderInit(Renderer** renderer) {
  Renderer* r = MemAllocZeroInit<Renderer>(MEM_ALLOC_HEAP);

  WNDCLASSEXW wc;
  MemZeroInit(&wc);
  wc.cbSize        = UINT(sizeof(wc));
  wc.style         = CS_CLASSDC;
  wc.lpfnWndProc   = &MainWndProc;
  wc.hInstance     = ::GetModuleHandle(nullptr);
  wc.lpszClassName = L"DX12_MainWindow";
  if (!::RegisterClassExW(&wc)) {
    return false;
  }

  r->wnd_ = ::CreateWindowW(
      wc.lpszClassName,
      L"Game",
      WS_OVERLAPPEDWINDOW,
      100,          // x
      100,          // y
      1280,         // width
      800,          // height
      nullptr,      // parent
      nullptr,      // menu
      wc.hInstance, // instance
      nullptr       // param
  );

  // ---

  // If we want to use the debug layer it must be initialized before we create any D3D device

  HRESULT err;

#ifdef DX12_ENABLE_DEBUG_LAYER
  err = D3D12GetDebugInterface(IID_PPV_ARGS(&r->debug_layer_));
  if (FAILED(err)) {
    g_dx12_log.LogWarning("cannot enable debug layer");
  } else {
    r->debug_layer_->EnableDebugLayer();
  }
#endif

  err = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&r->dev_));
  if (FAILED(err)) {
    g_dx12_log.LogError("cannot create D3D device");
    return false;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  if (r->debug_layer_) {
    // This will make it so that we'll get a debugger break if the debug layer finds an error
    ID3D12InfoQueue* info_queue = nullptr;
    r->dev_->QueryInterface(IID_PPV_ARGS(&info_queue));
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    info_queue->Release();
    r->debug_layer_->Release();
  }
#endif

  // Create the render target view descriptor heap
  // and compute the render target descriptor handle offsets

  D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc = {};
  rtv_desc_heap_desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_desc_heap_desc.NumDescriptors             = RENDERER_BACK_BUFFER_COUNT;
  err = r->dev_->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&r->rtv_desc_heap_));
  if (FAILED(err)) {
    return false;
  }

  UINT                        rtv_desc_size = r->dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rtv_base      = r->rtv_desc_heap_->GetCPUDescriptorHandleForHeapStart();
  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    r->rtv_handle_[i] = rtv_base;
    rtv_base.ptr += rtv_desc_size;
  }

  // Create a descriptor heap for the CBV/SRV/UAV data

  D3D12_DESCRIPTOR_HEAP_DESC cbv_desc_heap_desc = {};
  cbv_desc_heap_desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cbv_desc_heap_desc.NumDescriptors             = 1;
  err = r->dev_->CreateDescriptorHeap(&cbv_desc_heap_desc, IID_PPV_ARGS(&r->cbv_desc_heap_));
  if (FAILED(err)) {
    return false;
  }

  // Create a command queue

  D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
  cmd_queue_desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
  err                                     = r->dev_->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&r->cmd_queue_));
  if (FAILED(err)) {
    return false;
  }

  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    err = r->dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&r->cmd_allocator_[i]));
    if (FAILED(err)) {
      return false;
    }
  }

  err = r->dev_->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, r->cmd_allocator_[0], nullptr, IID_PPV_ARGS(&r->cmd_list_));
  if (FAILED(err)) {
    return false;
  }

  // Closing the command list doesn't make it unusable
  // we do this for consistency so that the first render
  // treats the command list the same

  err = r->cmd_list_->Close();
  if (FAILED(err)) {
    return false;
  }

  err = r->dev_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&r->fence_));
  if (FAILED(err)) {
    return false;
  }

  r->fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!r->fence_event_) {
    return false;
  }

  IDXGIFactory4* dxgi_factory4 = nullptr;
  // err = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory4));
  err = CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory4));
  if (FAILED(err)) {
    return false;
  }

  // https://devblogs.microsoft.com/directx/dxgi-flip-model/
  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
  MemZeroInit(&swap_chain_desc);
  swap_chain_desc.BufferCount        = RENDERER_BACK_BUFFER_COUNT;
  swap_chain_desc.Width              = 0;
  swap_chain_desc.Height             = 0;
  swap_chain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.SampleDesc.Count   = 1;
  swap_chain_desc.SampleDesc.Quality = 0;
  swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
  swap_chain_desc.Scaling            = DXGI_SCALING_STRETCH;
  swap_chain_desc.Stereo             = FALSE;

  IDXGISwapChain1* swap_chain1 = nullptr;
  err                          = dxgi_factory4->CreateSwapChainForHwnd(
      r->cmd_queue_, // For Direct3D 12 this is a pointer to a direct command queue
      r->wnd_,
      &swap_chain_desc,
      nullptr,
      nullptr,
      &swap_chain1);
  if (FAILED(err)) {
    return false;
  }
  err = swap_chain1->QueryInterface(IID_PPV_ARGS(&r->swap_chain_));
  if (FAILED(err)) {
    return false;
  }
  swap_chain1->Release();
  dxgi_factory4->Release();
  err = r->swap_chain_->SetMaximumFrameLatency(RENDERER_BACK_BUFFER_COUNT);
  if (FAILED(err)) {
    return false;
  }
  r->swap_chain_waitable_obj_ = r->swap_chain_->GetFrameLatencyWaitableObject();

  const wchar_t* buffer_names[] = {
    L"RTV Buffer 0",
    L"RTV Buffer 1",
    L"RTV Buffer 2",
  };

  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    ID3D12Resource* buffer = nullptr;
    err                    = r->swap_chain_->GetBuffer(i, IID_PPV_ARGS(&buffer));
    if (FAILED(err)) {
      return false;
    }
    r->dev_->CreateRenderTargetView(buffer, nullptr, r->rtv_handle_[i]);
    buffer->SetName(buffer_names[i % _countof(buffer_names)]);
    r->rtv_buffer_[i] = buffer;
  }

  // ---

  ::ShowWindow(r->wnd_, SW_SHOWDEFAULT);
  ::UpdateWindow(r->wnd_);

  // ---

  renderer[0] = r;
  return true;
}

bool game::RenderUpdateInput(Renderer& r) {
  // Non-blocking message loop
  MSG msg;
  for (; ::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE);) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    if (msg.message == WM_QUIT) {
      return false;
    }
  }
  return true;
}

bool game::RenderUpdateFrame(Renderer& r) {
  DWORD  wait_for_n = 1;
  HANDLE wait_for[] = {
    r.swap_chain_waitable_obj_, // This is for preventing the CPU from getting too far ahead
    nullptr,
  };

  // We use a circular buffer and need to make sure we're not trampling on a frame in flight

  const UINT32 frame_index = r.frame_number_ % RENDERER_BACK_BUFFER_COUNT;

  if (0 < r.fence_value_[frame_index]) {
    UINT64 completed = r.fence_->GetCompletedValue();
    if (completed < r.fence_value_[frame_index]) {
      r.fence_->SetEventOnCompletion(r.fence_value_[frame_index], r.fence_event_);
      wait_for_n  = 2;
      wait_for[1] = r.fence_event_;
    }
  }

  ::WaitForMultipleObjects(wait_for_n, wait_for, TRUE, INFINITE);

  // ---

  const UINT bbi = r.swap_chain_->GetCurrentBackBufferIndex(); // back buffer index

  ID3D12CommandAllocator* cmd_allocator = r.cmd_allocator_[frame_index];
  cmd_allocator->Reset();

  ID3D12GraphicsCommandList* cmd_list = r.cmd_list_;
  cmd_list->Reset(cmd_allocator, nullptr);

  D3D12_RESOURCE_BARRIER barrier = {}; // RTV transition barrier
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource   = r.rtv_buffer_[bbi];
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
  cmd_list->ResourceBarrier(1, &barrier);

  CD3DX12_VIEWPORT viewport(0.0f, 0.0f, 1280.0f, 800.0f);
  cmd_list->RSSetViewports(1, &viewport);

  // Pixels outside of the scissor rectangle are discarded
  CD3DX12_RECT scissor_rect(0, 0, 1280, 800);
  cmd_list->RSSetScissorRects(1, &scissor_rect);

  float f = 0.5f * sinf(2 * 3.1415926535897932384626433f * ((r.frame_number_ % 144) / 144.0f));

  const float clear_color_with_alpha[4] = { f, f, f, 1.0 }; // dark gray
  cmd_list->ClearRenderTargetView(r.rtv_handle_[bbi], clear_color_with_alpha, 0, nullptr);

  cmd_list->OMSetRenderTargets(1, &r.rtv_handle_[bbi], FALSE, nullptr);

  // insert actual drawing code here
  // insert actual drawing code here
  // insert actual drawing code here

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
  cmd_list->ResourceBarrier(1, &barrier);

  cmd_list->Close();

  ID3D12CommandList* cmd_lists[] = { cmd_list };
  r.cmd_queue_->ExecuteCommandLists(1, cmd_lists);

  // Since we created the swap chain passing in the DX12 command queue.
  // This doesn't present the frame it goes via the DX12 command queue
  // move this after the fence below and you will get an error when
  // we try to teardown the DX12 swap chain

  // https://learn.microsoft.com/en-us/windows/win32/direct3d12/swap-chains

  // With DX12 IDXGISwapChain::Present goes via the command queue
  // Note that CreateSwapChainForHwnd takes the command queue as a parameter at creation
  // If we signal our fence before Present() the operation is not included
  // If we signal out fence after Present() the operation is included

  r.swap_chain_->Present(1, 0);

  // ---

  UINT64 fence_value          = r.NextFenceValue();
  r.fence_value_[frame_index] = fence_value;
  r.cmd_queue_->Signal(r.fence_, fence_value);

  // ---

  r.frame_number_++; // we're finished

  return true;
}

bool game::RenderShutdown(Renderer& r) {
  // Wait for frame to complete

  const UINT32 frame_index = (r.frame_number_ - 1) % RENDERER_BACK_BUFFER_COUNT;

  if (0 < r.fence_value_[frame_index]) {
    UINT64 completed = r.fence_->GetCompletedValue();
    if (completed < r.fence_value_[frame_index]) {
      r.fence_->SetEventOnCompletion(r.fence_value_[frame_index], r.fence_event_);
      ::WaitForSingleObject(r.fence_event_, INFINITE);
    }
  }

  // Tear down DX12

  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    r.rtv_buffer_[i]->Release();
    r.rtv_buffer_[i] = nullptr;
  }

  if (r.swap_chain_) {
    // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#destroying-a-swap-chain
    r.swap_chain_->SetFullscreenState(false, nullptr);
    r.swap_chain_->Release();
    r.swap_chain_ = nullptr;
  }

  if (r.swap_chain_waitable_obj_) {
    CloseHandle(r.swap_chain_waitable_obj_);
    r.swap_chain_waitable_obj_ = INVALID_HANDLE_VALUE;
  }

  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    r.cmd_allocator_[i]->Release();
    r.cmd_allocator_[i] = nullptr;
  }

  if (r.cmd_queue_) {
    r.cmd_queue_->Release();
    r.cmd_queue_ = nullptr;
  }

  if (r.cmd_list_) {
    r.cmd_list_->Release();
    r.cmd_list_ = nullptr;
  }

  if (r.rtv_desc_heap_) {
    r.rtv_desc_heap_->Release();
    r.rtv_desc_heap_ = nullptr;
  }

  if (r.cbv_desc_heap_) {
    r.cbv_desc_heap_->Release();
    r.cbv_desc_heap_ = nullptr;
  }

  if (r.fence_) {
    r.fence_->Release();
    r.fence_ = nullptr;
  }

  if (r.fence_event_) {
    CloseHandle(r.fence_event_);
    r.fence_event_ = INVALID_HANDLE_VALUE;
  }

  if (r.dev_) {
    r.dev_->Release();
    r.dev_ = nullptr;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  IDXGIDebug1* dxgi_debug = nullptr;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)))) {
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    dxgi_debug->Release();
  }
#endif

  return true;
}
