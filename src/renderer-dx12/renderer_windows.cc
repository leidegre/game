#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include "renderer.hh"
#include "renderer.inl"

#include "../common/mem.hh"

#include "../loader/tex-loader.hh"

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

Logger g_dx12_log = { GAME_LOG_CATEGORY_DX12 };

// ---

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

  // todo: recover window position from file if possible

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

  // If we want to use the debug layer it must be enabled before we create any device

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
  r->rtv_desc_heap_->SetName(L"RTV Descriptor Heap");

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
  cbv_desc_heap_desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  err = r->dev_->CreateDescriptorHeap(&cbv_desc_heap_desc, IID_PPV_ARGS(&r->cbv_desc_heap_));
  if (FAILED(err)) {
    return false;
  }

  // Create a "direct" command queue

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
      r->cmd_queue_, // For Direct3D 12 this should be a pointer to a "direct" command queue
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

  // load grid texture data

  {
    CD3DX12_HEAP_PROPERTIES tex_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format

    TextureAsset tex1;
    LoadTextureFromFile(MEM_ALLOC_HEAP, "data/textures/debug/xz-grid-1024.png", &tex1);

    D3D12_RESOURCE_DESC tex1_desc = {};
    tex1_desc.MipLevels           = 1;
    tex1_desc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bpp 4-channel
    tex1_desc.Width               = 1024;
    tex1_desc.Height              = 1024;
    tex1_desc.Flags               = D3D12_RESOURCE_FLAG_NONE;
    tex1_desc.DepthOrArraySize    = 1;
    tex1_desc.SampleDesc.Count    = 1;
    tex1_desc.SampleDesc.Quality  = 0;
    tex1_desc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    err = r->dev_->CreateCommittedResource(
        &tex_heap,
        D3D12_HEAP_FLAG_NONE,
        &tex1_desc,
        D3D12_RESOURCE_STATE_COPY_DEST, // copy destination
        nullptr,
        IID_PPV_ARGS(&r->grid_xz_tex_));
    if (FAILED(err)) {
      return false;
    }
    r->grid_xz_tex_->SetName(L"xz-grid-1024.png (default)"); // ???

    UINT64 tex1_buffer_size;

    r->dev_->GetCopyableFootprints(&tex1_desc, 0, 1, 0, nullptr, nullptr, nullptr, &tex1_buffer_size);

    // this is like a staging area for the texture upload
    CD3DX12_RESOURCE_DESC tex_upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(tex1_buffer_size);

    CD3DX12_HEAP_PROPERTIES tex_heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    err = r->dev_->CreateCommittedResource(
        &tex_heap_upload,
        D3D12_HEAP_FLAG_NONE,
        &tex_upload_buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&r->grid_xz_tex_upload_));
    if (FAILED(err)) {
      return false;
    }
    r->grid_xz_tex_upload_->SetName(L"xz-grid-1024.png (upload)");

    // Ideally we copy directly into this buffer.
    void* tex_upload_ptr;
    r->grid_xz_tex_upload_->Map(0, nullptr, &tex_upload_ptr);
    memcpy(tex_upload_ptr, tex1.tex_data_, tex1.tex_data_size_);
    r->grid_xz_tex_upload_->Unmap(0, nullptr);

    ID3D12GraphicsCommandList& cmd_list = *r->cmd_list_;

    D3D12_TEXTURE_COPY_LOCATION dest = {};
    dest.pResource                   = r->grid_xz_tex_;
    dest.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest.SubresourceIndex            = 0;

    D3D12_TEXTURE_COPY_LOCATION src        = {};
    src.pResource                          = r->grid_xz_tex_upload_;
    src.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset             = 0;
    src.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM; // Your format here
    src.PlacedFootprint.Footprint.Width    = tex1.tex_width_;
    src.PlacedFootprint.Footprint.Height   = tex1.tex_height_;
    src.PlacedFootprint.Footprint.Depth    = 1;
    src.PlacedFootprint.Footprint.RowPitch = tex1.tex_width_ * 4; // Assuming a byte for each channel in RGBA

    cmd_list.Reset(r->cmd_allocator_[0], nullptr);

    cmd_list.CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        r->grid_xz_tex_, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmd_list.ResourceBarrier(1, &barrier);

    cmd_list.Close();

    r->cmd_queue_->ExecuteCommandLists(1, (ID3D12CommandList**)&r->cmd_list_);

    RenderFence(*r);

    RenderWaitForCurr(*r);
  }

  // ---

  ::ShowWindow(r->wnd_, SW_SHOWDEFAULT);
  ::UpdateWindow(r->wnd_);

  // ---

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(r->wnd_);
  ImGui_ImplDX12_Init(
      r->dev_,
      RENDERER_BACK_BUFFER_COUNT,
      DXGI_FORMAT_R8G8B8A8_UNORM,
      r->cbv_desc_heap_,
      r->cbv_desc_heap_->GetCPUDescriptorHandleForHeapStart(),
      r->cbv_desc_heap_->GetGPUDescriptorHandleForHeapStart());

  // ---

  r->view_eye_    = { 0, 0, -10 };
  r->view_target_ = { 0, 0, 0 };
  r->view_up_     = { 0, 1, 0 };

  r->view_ = math::LookAtLH(r->view_eye_, r->view_target_, r->view_up_);

  r->proj_fovy_   = HALF_PI; // 90 deg
  r->proj_aspect_ = 1280.0f / 800.0f;
  r->proj_near_   = 0.1f;
  r->proj_far_    = 100.0f;

  r->proj_ = math::PerspectiveFovLH(r->proj_fovy_, r->proj_aspect_, r->proj_near_, r->proj_far_);

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
    printf("%4x %8llx %8llx\n", msg.message, msg.lParam, msg.wParam);
    switch (msg.message) {
    case WM_KEYDOWN: {
      if (msg.wParam == 'W') {
        // add velocity in the camera forward direction
      }
      break;
    }
    case WM_KEYUP: {
      break;
    }
    }
  }
  return true;
}

bool game::RenderFrameBegin(Renderer& r) {
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

  ID3D12CommandAllocator* cmd_allocator = r.cmd_allocator_[frame_index];
  cmd_allocator->Reset();

  ID3D12GraphicsCommandList* cmd_list = r.cmd_list_;
  cmd_list->Reset(cmd_allocator, nullptr);

  const UINT bbi = r.swap_chain_->GetCurrentBackBufferIndex(); // back buffer index

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

  // float f = 0.25f * sinf(2 * 3.1415926535897932384626433f * ((r.frame_number_ % 144) / 144.0f));

  const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // dark gray
  cmd_list->ClearRenderTargetView(r.rtv_handle_[bbi], clear_color_with_alpha, 0, nullptr);

  cmd_list->OMSetRenderTargets(1, &r.rtv_handle_[bbi], FALSE, nullptr);

  // ---

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  static bool show_demo_window = true;
  ImGui::ShowDemoWindow(&show_demo_window);

  return true;
}

bool game::RenderFrameEnd(Renderer& r) {
  const UINT32 frame_index = r.frame_number_ % RENDERER_BACK_BUFFER_COUNT;

  ID3D12GraphicsCommandList* cmd_list = r.cmd_list_;

  // ---

  ImGui::Render(); // all ImGui commands must have been issued before this call

  cmd_list->SetDescriptorHeaps(1, &r.cbv_desc_heap_);

  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list);

  // ---

  const UINT bbi = r.swap_chain_->GetCurrentBackBufferIndex(); // back buffer index

  D3D12_RESOURCE_BARRIER barrier = {}; // RTV transition barrier
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource   = r.rtv_buffer_[bbi];
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
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

  RenderFence(r);

  // ---

  r.frame_number_++; // we're finished

  return true;
}

bool game::RenderUpdateFrame(Renderer& r) {
  RenderFrameBegin(r);
  RenderFrameEnd(r);
  return true;
}

u64 game::RenderFence(Renderer& r) {
  const UINT32 frame_index = r.frame_number_ % RENDERER_BACK_BUFFER_COUNT;

  UINT64 fence_value          = r.NextFenceValue();
  r.fence_value_[frame_index] = fence_value;
  r.cmd_queue_->Signal(r.fence_, fence_value);

  return fence_value;
}

bool game::RenderWaitForCurr(Renderer& r) {
  // Wait for frame to complete

  const UINT32 frame_index = r.frame_number_ % RENDERER_BACK_BUFFER_COUNT;

  if (0 < r.fence_value_[frame_index]) {
    UINT64 completed = r.fence_->GetCompletedValue();
    if (completed < r.fence_value_[frame_index]) {
      r.fence_->SetEventOnCompletion(r.fence_value_[frame_index], r.fence_event_);
      ::WaitForSingleObject(r.fence_event_, INFINITE);
    }
  }

  return true;
}

bool game::RenderWaitForPrev(Renderer& r) {
  // Wait for frame to complete

  const UINT32 frame_index = (r.frame_number_ - 1) % RENDERER_BACK_BUFFER_COUNT;

  if (0 < r.fence_value_[frame_index]) {
    UINT64 completed = r.fence_->GetCompletedValue();
    if (completed < r.fence_value_[frame_index]) {
      r.fence_->SetEventOnCompletion(r.fence_value_[frame_index], r.fence_event_);
      ::WaitForSingleObject(r.fence_event_, INFINITE);
    }
  }

  return true;
}

bool game::RenderShutdown(Renderer& r) {
  RenderWaitForPrev(r);

  // ---

  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  // ---

  // Tear down resources

  if (r.grid_xz_tex_) {
    r.grid_xz_tex_->Release();
    r.grid_xz_tex_ = nullptr;
  }

  if (r.grid_xz_tex_upload_) {
    r.grid_xz_tex_upload_->Release();
    r.grid_xz_tex_upload_ = nullptr;
  }

  // ---

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

  if (r.fence_event_) {
    CloseHandle(r.fence_event_);
    r.fence_event_ = INVALID_HANDLE_VALUE;
  }

  if (r.fence_) {
    r.fence_->Release();
    r.fence_ = nullptr;
  }

  if (r.cmd_list_) {
    r.cmd_list_->Release();
    r.cmd_list_ = nullptr;
  }

  for (int i = 0; i < RENDERER_BACK_BUFFER_COUNT; i++) {
    r.cmd_allocator_[i]->Release();
    r.cmd_allocator_[i] = nullptr;
  }

  if (r.cmd_queue_) {
    r.cmd_queue_->Release();
    r.cmd_queue_ = nullptr;
  }

  if (r.cbv_desc_heap_) {
    r.cbv_desc_heap_->Release();
    r.cbv_desc_heap_ = nullptr;
  }

  if (r.rtv_desc_heap_) {
    r.rtv_desc_heap_->Release();
    r.rtv_desc_heap_ = nullptr;
  }

  if (r.dev_) {
#ifdef DX12_ENABLE_DEBUG_LAYER
    // If SetBreakOnSeverity has been used we must disable it otherwise
    // we will get a lot of DXGI_ERROR_NOT_FOUND exceptions when calling ReportLiveObjects
    ID3D12Debug*     debug_layer;
    ID3D12InfoQueue* info_queue = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_layer)))) {
      r.dev_->QueryInterface(IID_PPV_ARGS(&info_queue));
      info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
      info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
      info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
      info_queue->Release();
      debug_layer->Release();
    }
#endif

    r.dev_->Release();
    r.dev_ = nullptr;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  IDXGIDebug1* dxgi_debug = nullptr;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)))) {
    DXGI_DEBUG_RLO_FLAGS flags = DXGI_DEBUG_RLO_FLAGS(
        DXGI_DEBUG_RLO_DETAIL |        // default
        DXGI_DEBUG_RLO_IGNORE_INTERNAL // something UWP
    );
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, flags);
    dxgi_debug->Release();
  }

  IDXGIInfoQueue* dxgi_info_queue = nullptr;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)))) {
    char   message_buffer[2048];
    SIZE_T message_size;
    for (UINT64 i = 0; i < dxgi_info_queue->GetNumStoredMessages(DXGI_DEBUG_ALL); i++) {
      DXGI_INFO_QUEUE_MESSAGE* m = (DXGI_INFO_QUEUE_MESSAGE*)message_buffer;
      message_size               = sizeof(message_buffer);
      dxgi_info_queue->GetMessageW(DXGI_DEBUG_ALL, i, m, &message_size);
      printf("%s\n", m->pDescription);
      r.dxgi_info_queue_message_count_++;
    }
  }
#endif

  return true;
}

uint32_t game::RenderDebugInfoQueueMessageCount(Renderer& r) {
  return r.dxgi_info_queue_message_count_;
}
