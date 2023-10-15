#include "../components/components.hh"
#include "../ecs/ecs.hh"
#include "../ecs/local-to-world-system.hh"
#include "../renderer-dx12/renderer.hh"

#include "box-rendering-system.hh"

#include <imgui.h>

using namespace game;

void ImGuiMat4(const char* label, mat4& m) {
  char tmp[128];

  vec4 r0 = m.Row(0);
  sprintf(tmp, "%s.r0", label);
  ImGui::InputFloat4(tmp, r0.Array(), nullptr, ImGuiInputTextFlags_ReadOnly);

  vec4 r1 = m.Row(1);
  sprintf(tmp, "%s.r1", label);
  ImGui::InputFloat4(tmp, r1.Array(), nullptr, ImGuiInputTextFlags_ReadOnly);

  vec4 r2 = m.Row(2);
  sprintf(tmp, "%s.r2", label);
  ImGui::InputFloat4(tmp, r2.Array(), nullptr, ImGuiInputTextFlags_ReadOnly);

  vec4 r3 = m.Row(3);
  sprintf(tmp, "%s.r3", label);
  ImGui::InputFloat4(tmp, r3.Array(), nullptr, ImGuiInputTextFlags_ReadOnly);
}

int main(int, char**) {
  World w;

  // initialize the world with out component type information
  // this information is static and it cannot change once set
  w.Create(GetComponentTypeInfoArray());

  EntityManager& m = w.EntityManager();

  Archetype* a = m.CreateArchetype({ //GetComponentTypeId<Translation>(),
                                     //  GetComponentTypeId<Rotation>(),
                                     //  GetComponentTypeId<Scale>(),
                                     GetComponentTypeId<LocalToWorld>() });

  a->label_ = "LocalToWorld";

  Entity entities[3];
  m.CreateEntities(a, entities, ArrayLength(entities));

  // m.SetComponentData(entities[0], Translation{ 1, 2, 3 });
  // m.SetComponentData(entities[1], Translation{ 4, 5, 6 });
  // m.SetComponentData(entities[2], Translation{ 7, 8, 9 });

  // m.SetComponentData(entities[0], Translation{ 0, 0, 0 });
  // m.SetComponentData(entities[1], Translation{ 0, 1, 0.50f });
  // m.SetComponentData(entities[2], Translation{ 1, 0, 0.75f });

  // m.SetComponentData(entities[0], Scale{ 1.0f });
  // m.SetComponentData(entities[1], Scale{ 1.0f });
  // m.SetComponentData(entities[2], Scale{ 1.0f });

  // Systems have different latency requirements
  // and they don't need to run sequentially in lockstep
  // For now we're doing everything in a serial fashion

  // The whole point of having an interface for the system base class
  // is that we only run them when their declared inputs change

  TRS_LocalToWorldSystem local_to_world_system{};
  SystemState&           local_to_world_system_state = w.Register(&local_to_world_system);

  local_to_world_system.OnCreate(local_to_world_system_state);

  Renderer* r;
  RenderInit(&r);

  BoxRenderingSystem box_rendering_system{};
  SystemState&       box_rendering_system_state = w.Register(&box_rendering_system);

  box_rendering_system.renderer_ = r;
  // box_rendering_system.OnCreate(box_rendering_system_state);

  for (; RenderUpdateInput(*r);) {
    local_to_world_system.OnUpdate(local_to_world_system_state);

    RenderFrameBegin(*r);

    static bool s_view_is_identity = false;
    static bool s_proj_is_identity = false;

    {
      ImGui::Begin("renderer");

      ImGui::Text("camera");

      ImGui::Checkbox("view identity", &s_view_is_identity);

      ImGui::SliderFloat3("eye", r->view_eye_.Array(), -10.0f, +10.0f);
      ImGui::SliderFloat3("target", r->view_target_.Array(), -10.0f, +10.0f);
      ImGui::SliderFloat3("up", r->view_up_.Array(), -10.0f, +10.0f);

      ImGui::Checkbox("proj identity", &s_proj_is_identity);

      ImGui::SliderFloat("fovy", &r->proj_fovy_, -PI, PI);
      ImGui::SliderFloat("aspect", &r->proj_aspect_, -2.0f, +2.0f);
      ImGui::SliderFloat("near", &r->proj_near_, -100.0f, +100.0f);
      ImGui::SliderFloat("far", &r->proj_far_, -100.0f, +100.0f);

      ImGui::Text("view");

      ImGuiMat4("v", r->view_);

      ImGui::Text("projection");

      ImGuiMat4("p", r->proj_);

      ImGui::Text("view*projection");

      ImGuiMat4("v*p", r->view_proj_);

      ImGui::End();
    }

    // ---

    if (s_view_is_identity) {
      r->view_ = mat4::Identity();
    } else {
      r->view_ = math::LookAtLH(r->view_eye_, r->view_target_, r->view_up_);
    }

    if (s_proj_is_identity) {
      r->proj_ = mat4::Identity();
    } else {
      r->proj_ = math::PerspectiveFovLH(r->proj_fovy_, r->proj_aspect_, r->proj_near_, r->proj_far_);
    }

    r->view_proj_ = Mul(r->proj_, r->view_);

    // ---

    // box_rendering_system.OnUpdate(box_rendering_system_state);

    RenderFrameEnd(*r);

    MemResetTemp(); // call this automatically between system transitions?
  }

  RenderWaitForPrev(*r);

  local_to_world_system.OnDestroy(local_to_world_system_state);

  // box_rendering_system.OnDestroy(box_rendering_system_state);

  RenderShutdown(*r);

  w.Destroy();
  return 0;
}
