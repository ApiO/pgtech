#pragma once

#include <runtime/types.h>

namespace pge
{
  struct PackageResource
  {
    struct Type {
      u32 name;
      u32 num_resources;
      u32 _resource_offset;
    };
    u32  _num_types;
  }; // followed by its data in memory
}