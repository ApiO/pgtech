#pragma once

#include <glm/glm.hpp>
#include <data/font.h>
#include <runtime/collection_types.h>
#include <engine/pge_types.h> // for TextAlign
#include <pose.h> // moche pose_types.h

namespace pge
{
  struct Text
  {
    Text(Allocator &a);

    struct PageRange
    {
      const FontResource *font;
      u32 page;
      i32 num_chars;
      u32 pad;
    };

    struct CharInfo
    {
      f32 vertices[8];
      wchar_t chr; // TODO: UTF8 ALERT
    };

    Pose      pose;
    char     *string;
    TextAlign align;
    i32       width;
    i32       height;
    const FontResource *font;
    Array<CharInfo>     char_infos;
    Array<PageRange>    page_ranges;
  };

  struct TextSystem : IdLookupTable<Text*>
  {
    TextSystem(Allocator &a);
    ~TextSystem();
  };
}