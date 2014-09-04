#pragma once

#include <runtime/types.h>

namespace pge
{
  enum ShapeType
  {
    SHAPE_TYPE_CIRCLE = 0,
    SHAPE_TYPE_CHAIN,
    SHAPE_TYPE_POLYGON,
    SHAPE_TYPE_BOX
  };

  struct ShapeResource
  {
    ShapeType _type;
    u32 _num_components;
  };
}