#pragma once

#include <runtime/types.h>

namespace pge
{
  enum TextureType
  {
    TEXTURE_TYPE_FULL = 0,
    TEXTURE_TYPE_TILESET,
    TEXTURE_TYPE_ATLAS,
  };

  struct TextureResource
  {
    struct Region {
      u32 name;
      u16 page;
      u16 rotated;
      i32 x, y;
      i32 width, height;
      i32 margin[4]; // top right bottom left
    };

    struct Page {
      i32 width;
      i32 height;
      u32 data_offset;
      u32 data_size;
    };

    u16 type;
    u16 num_regions;
    u16 num_empty_regions;
    u16 num_pages;
  }; // followed in memory by regions, empty regions, pages and page data */
}