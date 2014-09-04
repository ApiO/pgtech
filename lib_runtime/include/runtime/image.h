#pragma once

#include "types.h"
#include "memory_types.h"

namespace pge
{
  void rgba_to_bgra(u8 *data, const u32 size);
  void flip_region(u8 *data, u32 x, u32 y, u32 width, u32 height, u32 image_width, Allocator &a);
}