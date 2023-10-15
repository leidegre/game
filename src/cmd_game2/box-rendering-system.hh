#pragma once

#include "../renderer-dx12/renderer-inl.hh"

#include "../ecs/system.hh"

namespace game {
// The box rendering system will draw a vertex colored cube for every LocalToWorld component
struct BoxRenderingSystem : public System {
  Renderer*    renderer_;
  EntityQuery* query_;

  ID3D12Resource*      unit_cube_vertex_data_buffer_;
  u32                  unit_cube_vertex_data_buffer_size_;
  u32                  unit_cube_vertex_data_count_;
  ID3D12Resource*      cbv_;
  ID3DBlob*            vs_;
  ID3DBlob*            ps_;
  ID3D12RootSignature* root_signature_;
  ID3D12PipelineState* pso_;

  virtual void OnCreate(SystemState& state) override;
  virtual void OnUpdate(SystemState& state) override;
  virtual void OnDestroy(SystemState& state) override;
};
} // namespace game