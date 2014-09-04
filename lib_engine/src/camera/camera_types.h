#pragma once

#include <runtime/types.h>
#include <engine/pge_types.h>
#include <pose.h>
#include <culling/culling_types.h>

namespace pge
{
  struct Camera
  {
    Camera();
    bool           update_projection;
    glm::mat4      projection;
    glm::mat4      view;
    glm::mat4      projection_view;
    Frustum        frustum;
    ProjectionType projection_type;
    Pose pose;
    f32  far_range;
    f32  near_range;
    f32  left;
    f32  right;
    f32  bottom;
    f32  top;
    f32  fov;
    f32  aspect;
  };

  struct CameraSystem : IdLookupTable<Camera>
  {
    CameraSystem(Allocator &a);
    ~CameraSystem();
  };

}