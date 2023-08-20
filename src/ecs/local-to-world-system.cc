#include "local-to-world-system.hh"

#include "../components/components.hh"

#include "../math/transform.hh"

using namespace game;

namespace {
struct TRS_LocalToWorldJobData {
  ComponentDataReader<Translation>        translation_handle_;
  ComponentDataReader<Rotation>           rotation_handle_;
  ComponentDataReader<Scale>              scale_handle_;
  ComponentDataReaderWriter<LocalToWorld> local_to_world_handle_;
};

void TRS_LocalToWorldJobKernel(TRS_LocalToWorldJobData& data, const SystemChunk& chunk) {
  // todo: __restrict?

  const Translation* translation    = chunk.GetArray(data.translation_handle_); // optional
  const Rotation*    rotation       = chunk.GetArray(data.rotation_handle_);    // optional
  const Scale*       scale          = chunk.GetArray(data.scale_handle_);       // optional
  LocalToWorld*      local_to_world = chunk.GetArray(data.local_to_world_handle_);

  if (translation != nullptr) {
    if (rotation != nullptr) {
      if (scale != nullptr) {
        // TRS
        for (i32 i = 0; i < chunk.Len(); i++) {
          local_to_world[i].value_ = math::TRS(
              translation[i].value_, rotation[i].value_, { scale[i].value_, scale[i].value_, scale[i].value_ });
        }
      } else {
        // TR
      }
    } else {
      // Without rotation
      if (scale != nullptr) {
        // TS
      } else {
        // T
        for (i32 i = 0; i < chunk.Len(); i++) {
          local_to_world[i].value_ = math::Translation(translation[i].value_);
        }
      }
    }
  } else {
    // Without translation
    if (rotation != nullptr) {
      if (scale != nullptr) {
        // RS
      } else {
        // R
      }
    } else {
      // Without rotation
      if (scale != nullptr) {
        // S
        for (i32 i = 0; i < chunk.Len(); i++) {
          local_to_world[i].value_ = math::ScaleUniform(scale[i].value_);
        }
      } else {
        // Identity
        for (i32 i = 0; i < chunk.Len(); i++) {
          local_to_world[i].value_ = mat4::Identity();
        }
      }
    }
  }
}
} // namespace

void TRS_LocalToWorldSystem::OnCreate(SystemState& state) {
  q_ = state.EntityManager().CreateQuery({ ComponentDataAccess::Write<LocalToWorld>(),
                                           ComponentDataAccess::ReadAny<Translation>(),
                                           ComponentDataAccess::ReadAny<Rotation>(),
                                           ComponentDataAccess::ReadAny<Scale>() });
}

void TRS_LocalToWorldSystem::OnUpdate(SystemState& state) {
  TRS_LocalToWorldJobData data{};
  ExecuteJob(q_, data, TRS_LocalToWorldJobKernel);
}