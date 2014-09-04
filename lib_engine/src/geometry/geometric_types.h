#pragma once

#include <runtime/memory_types.h>
#include <runtime/collection_types.h>
#include <pose.h>

namespace pge
{
  struct Geometry
  {
    Pose pose;
    u8  *data;
    u32  size;
  };

  struct GeometricSystem : IdLookupTable<Geometry>
  {
    GeometricSystem(Allocator &a);
    ~GeometricSystem();
  };
}