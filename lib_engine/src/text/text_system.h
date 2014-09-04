#pragma once

#include <renderer/renderer_types.h>
#include "text_types.h"

namespace pge
{
  namespace text_system
  {
    u64  create   (TextSystem &system, const FontResource *font, const char *string, TextAlign align);
    void destroy  (TextSystem &system, u64 text);
    void update   (TextSystem &system);
    void gather   (TextSystem &system);
    void draw     (const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer);

    void get_size(TextSystem &system, u64 text, glm::vec2 &size);
    void get_width(TextSystem &system, u64 text, f32 &v);
    void get_height(TextSystem &system, u64 text, f32 &v);
    void set_string    (TextSystem &system, u64 text, const char *string);
    void set_font      (TextSystem &system, u64 text, const FontResource *font);
    void set_alignment (TextSystem &system, u64 text, TextAlign align);
    void set           (TextSystem &system, u64 text, const FontResource *font, const char *string, TextAlign align);

    Pose &get_pose     (TextSystem &system, u64 text);
  }

  inline Text::Text(Allocator &a) : char_infos(a), page_ranges(a), font(0), string(0) {}
  inline TextSystem::TextSystem(Allocator &a): IdLookupTable(a) {}
}