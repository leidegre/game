#include "box-rendering-system.hh"

#include "../components/components.hh"

using namespace game;

struct Vertex3 {
  f32 xyz_[3];
};

struct Vertex4 {
  f32 xyzw_[4];
};

struct VertexFormat {
  Vertex3 pos_;
  Vertex4 color_;
};

void BoxRenderingSystem::OnCreate(SystemState& state) {
  assert(renderer_);

  query_ = state.EntityManager().CreateQuery({ ComponentDataAccess::Read<LocalToWorld>() });

  Renderer& r = *renderer_;

  Vertex4 red     = { 1, 0, 0, 1 };
  Vertex4 green   = { 0, 1, 0, 1 };
  Vertex4 blue    = { 0, 0, 1, 1 };
  Vertex4 cyan    = { 0, 1, 1, 1 };
  Vertex4 magenta = { 1, 0, 1, 1 };
  Vertex4 yellow  = { 1, 1, 0, 1 };
  Vertex4 white   = { 1, 1, 1, 1 };

  // We're going to draw instanced boxes, we're going to define a unit cube
  // https://www.3dgep.com/wp-content/uploads/2018/03/Winding-Order.png

  // the cube is defined by 6 surfaces
  // that are made up from 8 vertices and 12 triangles

  Vertex3 v0 = { +1, +1, -1 };
  Vertex3 v1 = { -1, +1, -1 };
  Vertex3 v2 = { -1, -1, -1 };
  Vertex3 v3 = { +1, -1, -1 };
  Vertex3 v4 = { +1, +1, +1 };
  Vertex3 v5 = { -1, +1, +1 };
  Vertex3 v6 = { -1, -1, +1 };
  Vertex3 v7 = { +1, -1, +1 };

  Vertex3 front[4] = { v0, v1, v2, v3 };
  Vertex3 back[4]  = { v4, v5, v6, v7 };

  Vertex3 top[4]    = { v4, v5, v2, v0 };
  Vertex3 bottom[4] = { v7, v6, v2, v3 }; // ???

  Vertex3 left[4]  = { v1, v5, v6, v2 };
  Vertex3 right[4] = { v4, v0, v3, v7 };

  VertexFormat data[] = {
    {      v3, cyan}, // cyan
    {      v2, cyan},
    {      v1, cyan},

    {      v3, cyan}, // cyan
    {      v1, cyan},
    {      v4, cyan},

    {right[2],  red},
    {right[1],  red},
    {right[0],  red},

    {right[2],  red},
    {right[0],  red},
    {right[3],  red},

 // {{ +0, +0, 0.1f },   red},
  // {{ +0, +1, 0.1f },   red},
  // {{ -1, +1, 0.1f },   red},

  // {{ +0, +0, 0.1f }, green},
  // {{ +0, +1, 0.1f }, green},
  // {{ +1, +1, 0.1f }, green},

  // {{ +0, +0, 0.1f },  blue},
  // {{ +0, -1, 0.1f },  blue},
  // {{ -1, -1, 0.1f },  blue},

  // {{ +0, +0, 0.1f }, white},
  // {{ +0, -1, 0.1f }, white},
  // {{ +1, -1, 0.1f }, white},
  };

  // Maybe add more triangle data?

  D3D12_INPUT_ELEMENT_DESC vertex_data_layout[] = {
    {"POSITION", 0,    DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {   "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  CD3DX12_HEAP_PROPERTIES heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

  CD3DX12_RESOURCE_DESC vertex_data_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(data));

  r.dev_->CreateCommittedResource(
      &heap,
      D3D12_HEAP_FLAG_NONE,
      &vertex_data_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&unit_cube_vertex_data_buffer_));

  void*         data_ptr;
  CD3DX12_RANGE data_range(0, 0); // no read access, write only
  unit_cube_vertex_data_buffer_->Map(0, &data_range, &data_ptr);
  memcpy(data_ptr, data, sizeof(data));
  unit_cube_vertex_data_buffer_->Unmap(0, nullptr);
  unit_cube_vertex_data_buffer_size_ = sizeof(data);

  unit_cube_vertex_data_count_ = ArrayLength(data);

  CD3DX12_RESOURCE_DESC cbv_desc = CD3DX12_RESOURCE_DESC::Buffer(MemAlign(64 * 512, 256));

  r.dev_->CreateCommittedResource(
      &heap, //
      D3D12_HEAP_FLAG_NONE,
      &cbv_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&cbv_));

  // ---

  // Need to create root signature, shader and the PSO

  CD3DX12_ROOT_PARAMETER cbv[1];
  cbv[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

  CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.Init(1, cbv, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ID3DBlob* err;

  ID3DBlob* root_signature_blob;
  D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &err);
  r.dev_->CreateRootSignature(
      0, //
      root_signature_blob->GetBufferPointer(),
      root_signature_blob->GetBufferSize(),
      IID_PPV_ARGS(&root_signature_));

#if defined(_DEBUG)
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compile_flags = 0;
#endif

  D3DCompileFromFile(
      L"data/shaders/matrix-instanced.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compile_flags, 0, &vs_, &err);

  D3DCompileFromFile(
      L"data/shaders/matrix-instanced.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compile_flags, 0, &ps_, &err);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pos_desc{};
  pos_desc.InputLayout     = { vertex_data_layout, _countof(vertex_data_layout) };
  pos_desc.pRootSignature  = root_signature_;
  pos_desc.VS              = CD3DX12_SHADER_BYTECODE(vs_);
  pos_desc.PS              = CD3DX12_SHADER_BYTECODE(ps_);
  pos_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

  // For now disable some stuff

  pos_desc.RasterizerState.DepthClipEnable = FALSE;

  pos_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

  pos_desc.DepthStencilState.DepthEnable    = FALSE;
  pos_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  pos_desc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  pos_desc.DepthStencilState.StencilEnable  = FALSE;

  pos_desc.SampleMask            = UINT_MAX;
  pos_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pos_desc.NumRenderTargets      = 1;
  pos_desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
  pos_desc.SampleDesc.Count      = 1; // no multisampling

  pos_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

  r.dev_->CreateGraphicsPipelineState(&pos_desc, IID_PPV_ARGS(&pso_));
}

struct CopyLocalToWorldJobData {
  ComponentDataReader<LocalToWorld> local_to_world_;

  byte* buffer_;
  u32   buffer_offset_;
};

void CopyLocalToWorldJob(CopyLocalToWorldJobData& job, const SystemChunk& chunk) {
  const LocalToWorld* local_to_world = chunk.GetArray(job.local_to_world_);

  mat4 m[256]; // chunk size is 16 KiB, 16384/64=256

  // view projection matrix

  u32 size = chunk.Len() * sizeof(LocalToWorld);
  memcpy(job.buffer_ + job.buffer_offset_, local_to_world, size);
  job.buffer_offset_ += size;
}

void BoxRenderingSystem::OnUpdate(SystemState& state) {
  i32 n = query_->Count();

  // This will only work up to 512 cubes then everything breaks down

  // As an alternative to this job we could Map/Unmap the CBV
  // and copy the matrix data into the CBV directly without
  // going via the temporary buffer from within the job
  // That assumes that Map/Unmap is inexpensive

  byte* buffer = (byte*)MemAlloc(MEM_ALLOC_TEMP, 64 * 512, 256);

  memcpy(buffer, &renderer_->view_proj_, 64);

  CopyLocalToWorldJobData copy_local_to_world_job{};
  copy_local_to_world_job.buffer_        = buffer;
  copy_local_to_world_job.buffer_offset_ = 64; // view projection matrix
  ExecuteJob(query_, copy_local_to_world_job, CopyLocalToWorldJob);

  void* cbv_ptr;
  cbv_->Map(0, nullptr, &cbv_ptr);
  memcpy(cbv_ptr, buffer, copy_local_to_world_job.buffer_offset_);
  cbv_->Unmap(0, nullptr);

  // ---

  ID3D12GraphicsCommandList& cmd_list = *renderer_->cmd_list_;

  cmd_list.SetGraphicsRootSignature(root_signature_);
  cmd_list.SetPipelineState(pso_);

  cmd_list.SetGraphicsRootConstantBufferView(0, cbv_->GetGPUVirtualAddress());

  cmd_list.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  D3D12_VERTEX_BUFFER_VIEW vertex_data_view;
  vertex_data_view.BufferLocation = unit_cube_vertex_data_buffer_->GetGPUVirtualAddress();
  vertex_data_view.StrideInBytes  = sizeof(VertexFormat);
  vertex_data_view.SizeInBytes    = unit_cube_vertex_data_buffer_size_;
  cmd_list.IASetVertexBuffers(0, 1, &vertex_data_view);

  cmd_list.DrawInstanced(unit_cube_vertex_data_count_, n, 0, 0); // this will stop working after 512 cubes
}

void BoxRenderingSystem::OnDestroy(SystemState& state) {
  pso_->Release();
  vs_->Release();
  ps_->Release();
  root_signature_->Release();
  cbv_->Release();
  unit_cube_vertex_data_buffer_->Release();
}