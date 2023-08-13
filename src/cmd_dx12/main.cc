// The purpose of this file is to act as an introduction to DirectX 12
// Trying to map out everything that goes into drawing a frame

#include <d3d12.h>
#include <d3dx12.h> // helpers

// Microsoft DirectX Graphics Infrastructure (DXGI)
// https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi
// https://learn.microsoft.com/en-us/windows/win32/api/_direct3ddxgi
#include <dxgi1_4.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#endif

#include <d3dcompiler.h>

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

enum {
  GFX_BACK_BUFFER_COUNT     = 3,
  GFX_FRAME_IN_FLIGHT_COUNT = 3,
  GFX_NODE_MASK             = 0,
};

struct FrameContext {
  ID3D12CommandAllocator* CommandAllocator;
  UINT64                  FenceValue;
};

struct GfxState {
  FrameContext g_frameContext[GFX_FRAME_IN_FLIGHT_COUNT];
  UINT         g_frameIndex;

  ID3D12Device*              g_pd3dDevice;
  ID3D12DescriptorHeap*      g_pd3dRtvDescHeap;
  ID3D12DescriptorHeap*      g_pd3dSrvDescHeap;
  ID3D12CommandQueue*        g_pd3dCommandQueue;
  ID3D12GraphicsCommandList* g_pd3dCommandList;
  ID3D12Fence*               g_fence;
  HANDLE                     g_fenceEvent;

  IDXGISwapChain3* g_pSwapChain;
  HANDLE           g_hSwapChainWaitableObject;

  UINT64 g_fenceLastSignaledValue;

  ID3D12Resource*             g_mainRenderTargetResource[GFX_BACK_BUFFER_COUNT];
  D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[GFX_BACK_BUFFER_COUNT];
};

// to draw anything with DX12 we need 3 things, a root signature, a shader and a pipeline state object
struct GfxShader {
  ID3D12RootSignature* root_signature_;
  ID3D12PipelineState* pso_;
};

bool GfxCreateDevice(GfxState& state, HWND wnd);

bool GfxShaderCreate(GfxState& state, GfxShader* shader);

FrameContext* GfxWaitForNextFrameResources(GfxState& state);

int main() {
  GfxState state;
  memset(&state, 0, sizeof(state));

  WNDCLASSEXW wc = {
    sizeof(wc),               //
    CS_CLASSDC,               //
    WndProc,                  //
    0L,                       //
    0L,                       //
    GetModuleHandle(nullptr), //
    nullptr,                  //
    nullptr,                  //
    nullptr,                  //
    nullptr,                  //
    L"DX12",                  //
    nullptr,                  //
  };

  ::RegisterClassExW(&wc);
  HWND hwnd = ::CreateWindowW(
      wc.lpszClassName, //
      L"DX12",
      WS_OVERLAPPEDWINDOW,
      100,
      100,
      1280,
      800,
      nullptr,
      nullptr,
      wc.hInstance,
      nullptr);

  if (!GfxCreateDevice(state, hwnd)) {
    // CleanupDeviceD3D();
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  // ---

  // For our triangle demo we need some data
  // we'll put this data in an upload heap
  // this is not optimal but it is simple to do
  //
  // the problem with the upload heap is that each
  // time the GPU needs to read the memory it will
  // be copied. it won't stay on the GPU

  struct VertexData {
    float xyz_[3];
    float rgba_[4];
  };

  VertexData vertex_data[] = {
    {{ +0, +1, 0 }, { 1, 0, 0, 1 }}, // top center
    {{ +1, -1, 0 }, { 0, 0, 1, 1 }}, // right bottom
    {{ -1, -1, 0 }, { 0, 1, 0, 1 }}, // left bottom
  };

  ID3D12Resource* m_vertexBuffer;

  CD3DX12_HEAP_PROPERTIES upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC   buffer      = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex_data));

  state.g_pd3dDevice->CreateCommittedResource(
      &upload_heap,
      D3D12_HEAP_FLAG_NONE,
      &buffer,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_vertexBuffer));

  // Copy the triangle data to the vertex buffer.
  UINT8*        pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
  m_vertexBuffer->Map(0, &readRange, (void**)&pVertexDataBegin);
  memcpy(pVertexDataBegin, vertex_data, sizeof(vertex_data));
  m_vertexBuffer->Unmap(0, nullptr);

  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.StrideInBytes  = sizeof(VertexData);
  m_vertexBufferView.SizeInBytes    = sizeof(vertex_data);

  // We need syncronization of this upload... the hello triangle DEMO does this via WaitForPreviousFrame
  // I'm assuming that GfxWaitForNextFrameResources does the same thing for us

  // Create an empty root signature.
  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ID3DBlob* signature;
  ID3DBlob* error;
  D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  ID3D12RootSignature* m_rootSignature;
  state.g_pd3dDevice->CreateRootSignature(
      0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

  ID3D12PipelineState* m_pipelineState;

  // Create the pipeline state, which includes compiling and loading shaders.
  {
    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    D3DCompileFromFile(
        L"data/shaders/basic.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
    D3DCompileFromFile(
        L"data/shaders/basic.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature                     = m_rootSignature;
    psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(vertexShader);
    psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(pixelShader);
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable      = FALSE;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.RTVFormats[0]                      = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count                   = 1;
    state.g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
  }

  GfxShader matrix_shader;
  GfxShaderCreate(state, &matrix_shader);

  // ---

  // The "matrix" shader relies on a constant buffer to receive the transformation matrix

  // We can reuse our upload heap

  // constant buffers must be at multiple of 256 bytes

  CD3DX12_RESOURCE_DESC matrix_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(256); // 1x float4x4 matrix
  ID3D12Resource*       matrix_constant_buffer;

  state.g_pd3dDevice->CreateCommittedResource(
      &upload_heap,
      D3D12_HEAP_FLAG_NONE,
      &matrix_buffer_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&matrix_constant_buffer));

  // This is only needed if we go via descriptor tables
  // D3D12_CONSTANT_BUFFER_VIEW_DESC matrix_constant_buffer_desc = {};
  // matrix_constant_buffer_desc.BufferLocation =
  //     matrix_constant_buffer->GetGPUVirtualAddress(); // assuming 'constantBuffer' is your ID3D12Resource*
  // matrix_constant_buffer_desc.SizeInBytes = 256;
  // state.g_pd3dDevice->CreateConstantBufferView(
  //     &matrix_constant_buffer_desc, state.g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart());

  // ---

  void* matrix_constant_buffer_ptr;
  matrix_constant_buffer->Map(0, nullptr, &matrix_constant_buffer_ptr);
  float m[16] = {
    0.5, 0,   0,   0, // column 0
    0,   0.5, 0,   0, // column 1
    0,   0,   0.5, 0, // column 2
    0,   0,   0,   1, // column 3
  };
  memcpy(matrix_constant_buffer_ptr, m, 64);
  matrix_constant_buffer->Unmap(0, nullptr);

  // ---

  // Main loop
  bool isRunning = true;
  while (isRunning) {
    // Poll and handle messages (inputs, window resize, etc.)
    // See the WndProc() function below for our to dispatch events to the Win32 backend.
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      if (msg.message == WM_QUIT) {
        isRunning = false;
        break;
      }
    }

    // Render frame (each frame is associated with it's own command allocator)

    FrameContext* frameCtx      = GfxWaitForNextFrameResources(state);
    UINT          backBufferIdx = state.g_pSwapChain->GetCurrentBackBufferIndex();
    frameCtx->CommandAllocator->Reset();

    // Then we setup this barrier to basically coordinate a full sync of everything?

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = state.g_mainRenderTargetResource[backBufferIdx];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    state.g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
    state.g_pd3dCommandList->ResourceBarrier(
        1,
        &barrier); // I'm thinking this means that we will wait for the render target to reach the render target state here

    CD3DX12_VIEWPORT m_viewport(0.0f, 0.0f, 1280.0f, 800.0f);
    state.g_pd3dCommandList->RSSetViewports(1, &m_viewport);

    // Pixels outside of the scissor rectangle are discarded
    CD3DX12_RECT m_scissorRect(0, 0, 1280, 800);
    state.g_pd3dCommandList->RSSetScissorRects(1, &m_scissorRect);

    // And now we can actually issue draw commands
    const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0 }; // dark gray

    state.g_pd3dCommandList->ClearRenderTargetView(
        state.g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);

    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-omsetrendertargets
    state.g_pd3dCommandList->OMSetRenderTargets(1, &state.g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);

    // state.g_pd3dCommandList->SetDescriptorHeaps(1, &state.g_pd3dSrvDescHeap);

    // ---

    // state.g_pd3dCommandList->SetGraphicsRootSignature(m_rootSignature);
    // state.g_pd3dCommandList->SetPipelineState(m_pipelineState);

    state.g_pd3dCommandList->SetGraphicsRootSignature(matrix_shader.root_signature_);
    state.g_pd3dCommandList->SetPipelineState(matrix_shader.pso_);

    // state.g_pd3dCommandList->SetGraphicsRootDescriptorTable(
    //     0, state.g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    state.g_pd3dCommandList->SetGraphicsRootConstantBufferView(0, matrix_constant_buffer->GetGPUVirtualAddress());

    state.g_pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    state.g_pd3dCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    state.g_pd3dCommandList->DrawInstanced(3, 1, 0, 0);

    // ---

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#resource-states-for-presenting-back-buffers
    // and now we wait for the render target to transition into the present state?

    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    state.g_pd3dCommandList->ResourceBarrier(1, &barrier);
    state.g_pd3dCommandList->Close();

    state.g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&state.g_pd3dCommandList);

    state.g_pSwapChain->Present(1, 0); // Present with vsync
    //g_pSwapChain->Present(0, 0); // Present without vsync

    UINT64 fenceValue = state.g_fenceLastSignaledValue + 1;
    state.g_pd3dCommandQueue->Signal(state.g_fence, fenceValue);
    state.g_fenceLastSignaledValue = fenceValue;
    frameCtx->FenceValue           = fenceValue;
  }

  return 0;
}

bool GfxCreateDevice(GfxState& state, HWND wnd) {
  // Step 1. Setup a swap chain

  DXGI_SWAP_CHAIN_DESC1 sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount        = GFX_BACK_BUFFER_COUNT;
  sd.Width              = 0;
  sd.Height             = 0;
  sd.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.Flags              = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.SampleDesc.Count   = 1;
  sd.SampleDesc.Quality = 0;
  sd.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
  sd.Scaling            = DXGI_SCALING_STRETCH;
  sd.Stereo             = FALSE;

#ifdef DX12_ENABLE_DEBUG_LAYER
  ID3D12Debug* pdx12Debug = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug)))) {
    pdx12Debug->EnableDebugLayer();
  }
#endif

  // Create device
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&state.g_pd3dDevice)) != S_OK) {
    return false;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  if (pdx12Debug != nullptr) {
    ID3D12InfoQueue* pInfoQueue = nullptr;
    state.g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    pInfoQueue->Release();
    pdx12Debug->Release();
  }
#endif

  // Step 2. Create descriptor heaps

  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // render target view
    desc.NumDescriptors             = GFX_BACK_BUFFER_COUNT;
    desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (state.g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&state.g_pd3dRtvDescHeap)) != S_OK) {
      return false;
    }

    SIZE_T rtvDescriptorSize = state.g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = state.g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < GFX_BACK_BUFFER_COUNT; i++) {
      state.g_mainRenderTargetDescriptor[i] = rtvHandle;
      rtvHandle.ptr += rtvDescriptorSize;
    }
  }

  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors             = 1;
    desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (state.g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&state.g_pd3dSrvDescHeap)) != S_OK)
      return false;
  }

  {
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (state.g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&state.g_pd3dCommandQueue)) != S_OK)
      return false;
  }

  for (UINT i = 0; i < GFX_FRAME_IN_FLIGHT_COUNT; i++) {
    if (state.g_pd3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&state.g_frameContext[i].CommandAllocator))
        != S_OK) {
      return false;
    }
  }

  if (state.g_pd3dDevice->CreateCommandList(
          0,
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          state.g_frameContext[0].CommandAllocator,
          nullptr,
          IID_PPV_ARGS(&state.g_pd3dCommandList))
      != S_OK) {
    return false;
  }

  if (state.g_pd3dCommandList->Close() != S_OK) {
    return false;
  }

  if (state.g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&state.g_fence)) != S_OK) {
    return false;
  }

  state.g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (state.g_fenceEvent == nullptr) {
    return false;
  }

  {
    IDXGIFactory4*   dxgiFactory = nullptr;
    IDXGISwapChain1* swapChain1  = nullptr;
    if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
      return false;
    if (dxgiFactory->CreateSwapChainForHwnd(state.g_pd3dCommandQueue, wnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
      return false;
    if (swapChain1->QueryInterface(IID_PPV_ARGS(&state.g_pSwapChain)) != S_OK)
      return false;
    swapChain1->Release();
    dxgiFactory->Release();
    state.g_pSwapChain->SetMaximumFrameLatency(GFX_BACK_BUFFER_COUNT);
    state.g_hSwapChainWaitableObject = state.g_pSwapChain->GetFrameLatencyWaitableObject();
  }

  for (UINT i = 0; i < GFX_BACK_BUFFER_COUNT; i++) {
    ID3D12Resource* pBackBuffer = nullptr;
    state.g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
    state.g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, state.g_mainRenderTargetDescriptor[i]);
    state.g_mainRenderTargetResource[i] = pBackBuffer;
  }

  return 1;
}

FrameContext* GfxWaitForNextFrameResources(GfxState& state) {
  UINT nextFrameIndex = state.g_frameIndex + 1;
  state.g_frameIndex  = nextFrameIndex;

  HANDLE waitableObjects[]  = { state.g_hSwapChainWaitableObject, nullptr };
  DWORD  numWaitableObjects = 1;

  // if fence value is non zero we add a wait condition for the g_fence event... why that?

  FrameContext* frameCtx   = &state.g_frameContext[nextFrameIndex % GFX_FRAME_IN_FLIGHT_COUNT];
  UINT64        fenceValue = frameCtx->FenceValue;
  if (fenceValue != 0) // means no fence was signaled
  {
    frameCtx->FenceValue = 0;
    state.g_fence->SetEventOnCompletion(fenceValue, state.g_fenceEvent);
    waitableObjects[1] = state.g_fenceEvent;
    numWaitableObjects = 2;
  }

  ::WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

  return frameCtx;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_SIZE:
    // Swap chain may need resizing...
    // if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
    //   WaitForLastSubmittedFrame();
    //   CleanupRenderTarget();
    //   HRESULT result = g_pSwapChain->ResizeBuffers(
    //       0,
    //       (UINT)LOWORD(lParam),
    //       (UINT)HIWORD(lParam),
    //       DXGI_FORMAT_UNKNOWN,
    //       DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    //   assert(SUCCEEDED(result) && "Failed to resize swapchain.");
    //   CreateRenderTarget();
    // }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// The root signature cannot be part of this helper function

bool GfxShaderCreate(GfxState& state, GfxShader* shader) {
  HRESULT   hr;
  ID3DBlob* err;

  CD3DX12_DESCRIPTOR_RANGE root_param_desc_range;
  root_param_desc_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 1 constant buffer view at register b0

  CD3DX12_ROOT_PARAMETER root_param;
  // root_param.InitAsDescriptorTable(1, &root_param_desc_range);
  root_param.InitAsConstantBufferView(0); // ???

  CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.Init(1, &root_param, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ID3DBlob* root_signature_blob;
  D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &err);

  state.g_pd3dDevice->CreateRootSignature(
      0,                                       //
      root_signature_blob->GetBufferPointer(), //
      root_signature_blob->GetBufferSize(),    //
      IID_PPV_ARGS(&shader->root_signature_)   //
  );

#if defined(_DEBUG)
  UINT compiler_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compiler_flags = 0;
#endif

  ID3DBlob* vertex_shader;
  ID3DBlob* pixel_shader;

  hr = D3DCompileFromFile(
      L"data/shaders/matrix.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compiler_flags, 0, &vertex_shader, &err);

  if (FAILED(hr)) {
    printf("%*s\n", (int)err->GetBufferSize(), (const char*)err->GetBufferPointer());
  }

  D3DCompileFromFile(
      L"data/shaders/matrix.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compiler_flags, 0, &pixel_shader, &err);

  if (FAILED(hr)) {
    printf("%*s\n", (int)err->GetBufferSize(), (const char*)err->GetBufferPointer());
  }

  // Define the vertex input layout.
  D3D12_INPUT_ELEMENT_DESC input_layout[] = {
    {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

  pso_desc.InputLayout                     = { input_layout, _countof(input_layout) };
  pso_desc.pRootSignature                  = shader->root_signature_;
  pso_desc.VS                              = CD3DX12_SHADER_BYTECODE(vertex_shader);
  pso_desc.PS                              = CD3DX12_SHADER_BYTECODE(pixel_shader);
  pso_desc.RasterizerState                 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pso_desc.BlendState                      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pso_desc.DepthStencilState.DepthEnable   = FALSE;
  pso_desc.DepthStencilState.StencilEnable = FALSE;
  pso_desc.SampleMask                      = UINT_MAX;
  pso_desc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets                = 1;
  pso_desc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.SampleDesc.Count                = 1;

  state.g_pd3dDevice->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&shader->pso_));

  return true;
}
