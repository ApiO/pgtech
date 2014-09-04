#pragma once

#include <runtime/types.h>

namespace pge
{
  struct ParticleResource
  {
    bool  loop;
    f32   rate;
    f32   lifespan;
    f32   speed;
    f32   angle;
    u32   texture_name;
    u32   texture_region;
    f32   box        [3];
    u8    color_start[4];
    u8    color_end  [4];
    f32   scale_start[3];
    f32   scale_end  [3];
    f32   gravity    [3];
  };
}