#pragma once

#include <runtime/types.h>

namespace pge
{
  struct MoverResource {
    u32 name;
    f32 height;
    f32 radius;
    f32 slope_limit;
    f32 step_offset;
    f32 offset[2];
    u32 collision_filter;
  };
}