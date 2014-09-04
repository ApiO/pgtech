#pragma once

#include "runtime/types.h"
#include "runtime/collection_types.h"

namespace pge
{
  struct InputRectangle
  {
    i32 width, height;
  };

  struct PackedRectangle
  {
    i32  input;
    i32  x, y;
    i32  page;
    bool rotated;
  };

  struct RectanglePage
  {
    i32 width, height;
    i32 first_rectangle;
    u32 num_rectangles;
  };

  namespace texture_packer
  {
    bool process(
      const Array<InputRectangle> &input_rectangles,
      Array<PackedRectangle>      &output_rectangles,
      Array<RectanglePage>        &output_pages,
      i32  edge_max, 
      bool allow_rotation,
      u32  spacing,
      Allocator &a);
  }
}