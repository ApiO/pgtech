#pragma once

#include <runtime/types.h>

#include "types.h"

namespace pge
{
  struct LevelResource
  {
    struct Resource
    {
      u32 name;
      PoseResource pose;
    };
    u32 num_units;
    u32 num_sprites;
    u32 units_offset;
    u32 sprites_offset;
  };
}