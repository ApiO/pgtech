#pragma once

#include <runtime/types.h>
#include "types.h"

namespace pge
{
  struct ActorResource
  {
    struct Shape
    {
      u32 _instance_name;
      u32 _material;
      u32 _template;
      u32 _shape;
      PoseResource _pose;
    };
    u32 _actor;
    u32 _num_shapes;
  };

}