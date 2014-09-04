#pragma once

#include <runtime/types.h>

namespace pge
{
  struct Char
  {
    i16 x, y;
    i16 width, height;
    i16 offset_x, offset_y;
    i16 x_advance;
    u16 page;
    f32 tex_coord[8];
  };

  struct CharResource
  {
    i32  id;
    Char chr;
  };

  // Hash<FontResource::Char>
  struct FontResource
  {
    struct Page
    {
      u32 size;
      u32 offset;
    };
    i32 _line_height;
    //i32 _pages_width;
    //i32 _pages_height;
    u32 _num_chars;
    u32 _num_pages;
    u32 _chars_offset;
    u32 _pages_offset;
  };
  // *CharResource
  // *FontResource::Page
  // *u8 dds

}