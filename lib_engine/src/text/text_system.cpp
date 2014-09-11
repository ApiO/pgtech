#include <GL/glew.h>
#include <GL/GL.h>

#include <runtime/types.h>
#include <runtime/array.h>
#include <runtime/hash.h>
#include <runtime/array.h>
#include <runtime/sort.h>
#include <runtime/murmur_hash.h>
#include <runtime/tree.h>
#include <runtime/trace.h>
#include <runtime/temp_allocator.h>
#include <renderer/renderer.h>
#include <pose.h>

#include <utils/ogl_debug.h>
#include "text_system.h"
#include "font_resource.h"

namespace
{
  using namespace pge;

#define is_ignored_char(c)((c) == '\b' || (c) == '\f' || (c) == '\r' || (c) == '\t' || (c) == '\v')

  struct TextData
  {
    u32 num_chars;
    u32 num_ranges;
    u32 ranges_offest;
    u32 vertices_offest;
    u32 texcoord_offest;
    u32 colors_offest;
    u32 indices_offest;
  };

  struct CharPose
  {
    u32     page;
    f32     x;
    u32     line_index;
    wchar_t chr;
  };

  struct LineInfo
  {
    f32 offset_x;
    f32 offset_y;
  };

  const wchar_t MISSING_CHAR = L'?';
  const u32 VEC3_SIZE = sizeof(f32)* 3;

  bool compare_sort_key(const CharPose &a, const CharPose &b)
  {
    return a.page < b.page;
  }

  inline void setup_char_poses(Text *text, CharPose *char_poses, LineInfo *line_infos, Hash<u16> &pages, u32 &num_lines)
  {
    wchar_t wchar, wctmp;

    f32 max_width = 0.f,
        curr_width = 0.f;

    i32 line_index  = 0,
        curr_chars  = 0;

    LineInfo *line_info = line_infos;

    const char *start = text->string;
    const char *send  = text->string + strlen(text->string);
    f32 line_height   = font_resource::line_height(text->font) * text->scale;

    while (start < send) {
      start += mbtowc(&wctmp, start, start - send);

      if (is_ignored_char(start[0])) continue;

      // LINE INFO
      if (wctmp == L'\n') {
        line_info->offset_x = curr_width;
        line_info->offset_y = -(line_height * line_index);
        line_info++;
        line_index++;
        if (max_width < curr_width) max_width = curr_width;
        curr_width = 0;
        continue;
      }

      // CHAR POSE
      wchar = font_resource::has(text->font, (u64)wctmp) ? wctmp : MISSING_CHAR;
      const Char &chr = font_resource::get(text->font, (u64)wchar);

      CharPose &char_pose  = char_poses[curr_chars];
      char_pose.page       = chr.page;
      char_pose.x          = curr_width;
      char_pose.line_index = line_index;
      char_pose.chr        = wchar;

      hash::set(pages, (u64)chr.page, chr.page);

      curr_width += chr.x_advance * text->scale;
      curr_chars++;
    }
    line_info->offset_x = curr_width;
    line_info->offset_y = -(line_height * line_index) ;

    if (max_width < curr_width) max_width = curr_width;

    text->width  = max_width;
    text->height = font_resource::line_height(text->font) * num_lines * text->scale;
  }

  inline void set_vertices(Text *text, CharPose *char_poses, LineInfo *line_infos, u32 num_lines, u32 num_chars, Allocator &a)
  {
    // SETS ALIGNMENT
    {
      f32 pad_value = 0.f;
      switch (text->align) {
        case TEXT_ALIGN_LEFT:   pad_value = 0.f; break;
        case TEXT_ALIGN_RIGHT:  pad_value = 1.f; break;
        case TEXT_ALIGN_CENTER: pad_value = .5f; break;
      }
      for (u32 i = 0; i < num_lines; i++) {
        LineInfo  &line_info = line_infos[i];
        line_info.offset_x = ((text->width - line_info.offset_x) * pad_value);
      }
    }

    // SORTS CHARS BY PAGE INDEX
    CharPose *sort_buf = (CharPose*)a.allocate(sizeof(CharPose)* num_chars);
    merge_sort(char_poses, sort_buf, num_chars, compare_sort_key);
    a.deallocate(sort_buf);

    // SETS VERTICES
    array::resize(text->char_infos, num_chars);

    Text::CharInfo *char_info = array::begin(text->char_infos);

    for (u32 i = 0; i < num_chars; i++) {
      const CharPose &char_pose = char_poses[i];
      const Char &chr = font_resource::get(text->font, (u64)char_pose.chr);
      const LineInfo &line_info = line_infos[char_poses[i].line_index];

      char_info->chr = char_pose.chr;

      f32 x = (f32)(chr.offset_x + line_info.offset_x + char_pose.x);
      f32 y = (f32)(-chr.offset_y + line_info.offset_y);

      char_info->vertices[0] = x * text->scale;
      char_info->vertices[1] = (y - chr.height) * text->scale;
      char_info->vertices[2] = x * text->scale;
      char_info->vertices[3] = y * text->scale;
      char_info->vertices[4] = (x + chr.width) * text->scale;
      char_info->vertices[5] = y * text->scale;
      char_info->vertices[6] = (x + chr.width) * text->scale;
      char_info->vertices[7] = (y - chr.height) * text->scale;

      char_info++;
    }
  }

  inline void set_ranges(Text *text, Array<CharPose> &char_poses, Hash<u16> &pages)
  {
    array::resize(text->page_ranges, hash::size(pages));

    Text::PageRange *range = array::begin(text->page_ranges);
    const CharPose *char_pose = array::begin(char_poses),
      *cend =  array::end(char_poses);

    u32  curr_pad  = 0u;
    bool range_set = false;

    for (; char_pose < cend; char_pose++) {
      if (!range_set) {
        range->page = char_pose->page;
        range->num_chars = 0u;
        range->pad = 0u;
        range->font = text->font;
        range_set = true;
      }
      if (range->page != char_pose->page) {
        range++;
        range->font = text->font;
        range->page = char_pose->page;
        range->num_chars = 0u;
        range->pad = curr_pad;
      }
      range->num_chars++;
      curr_pad += 6;
    }
  }

  inline void _set_text(Text *text)
  {
    Allocator &a = memory_globals::default_allocator();
    Hash<u16> pages(a);
    Array<CharPose> char_poses(a);
    Array<LineInfo> line_infos(a);

    // Removes special chars
    u32 len = strlen(text->string);
    u32 num_lines = 1;
    u32 num_chars = 0;

    for (u32 i = 0; i < len; i++) {
      if (is_ignored_char(text->string[i])) continue;

      if (text->string[i] == '\n') {
        num_lines++;
        continue;
      }
      num_chars++;
    }

    // Reserves containers
    array::resize(char_poses, num_chars);
    array::resize(line_infos, num_lines);

    // Setups char poses
    setup_char_poses(text, char_poses._data, line_infos._data, pages, num_lines);

    // Sets vertices
    set_vertices(text, char_poses._data, line_infos._data, num_lines, num_chars, a);

    // sets ranges
    set_ranges(text, char_poses, pages);
  }
}

namespace pge
{
  namespace text_system
  {
    u64 create(TextSystem &system, const FontResource *font, const char *string, TextAlign align, f32 scale, const Color &color)
    {
      Text *text  = MAKE_NEW(*system._data._allocator, Text, *system._data._allocator);
      text->align = align;
      text->font  = font;
      text->scale = scale;
      text->color = color;

      if (string) {
        text->string = (char*)system._data._allocator->allocate(sizeof(char)*strlen(string));
        strcpy(text->string, string);

        _set_text(text);
      }
      return idlut::add(system, text);
    }

    void destroy(TextSystem &system, u64 text)
    {
      Allocator &a = *system._data._allocator;
      Text *t = *idlut::lookup(system, text);
      a.deallocate(t->string);
      MAKE_DELETE(a, Text, t);
      idlut::remove(system, text);
    }

    void update(TextSystem &system)
    {
      glm::mat4 m;
      TextSystem::Entry *e, *end = idlut::end(system);
      for (e = idlut::begin(system); e < end; e++)
        pose::update(e->value->pose);
    }

    void gather(TextSystem &system)
    {
      const TextSystem::Entry *e, *end = idlut::end(system);
      for (e = idlut::begin(system); e < end; e++) {

        const Text *text = e->value;

        if (!text->string) continue;

        glm::mat4 world_pose;
        pose::get_world_pose(text->pose, world_pose);

        glm::vec3 translation;
        pose::get_world_translation(text->pose, translation);

        SortKey sort_key;
        sort_key.z = translation.z;
        sort_key.value = 1000u;

        // Allocates & inits TextData
        TextData *text_data = NULL;
        u8 *data = NULL;
        {
          u32 num_chars = array::size(text->char_infos);
          u32 data_size = sizeof(TextData) +
            num_chars * (SPRITE_VERTICES_SIZE + SPRITE_TEXCOORD_SIZE + SPRITE_COLORS_SIZE + SPRITE_INDICES_SIZE) +
            array::size(text->page_ranges) * sizeof(Text::PageRange);

          data = renderer::create_graphic(GRAPHIC_TYPE_TEXT, data_size, sort_key);
          text_data = (TextData*)data;

          text_data->num_chars = num_chars;
          text_data->num_ranges = array::size(text->page_ranges);
          text_data->ranges_offest = sizeof(TextData);
          text_data->vertices_offest = text_data->ranges_offest + array::size(text->page_ranges) * sizeof(Text::PageRange);
          text_data->texcoord_offest = text_data->vertices_offest + num_chars * SPRITE_VERTICES_SIZE;
          text_data->colors_offest = text_data->texcoord_offest + num_chars * SPRITE_TEXCOORD_SIZE;
          text_data->indices_offest = text_data->colors_offest + num_chars * SPRITE_COLORS_SIZE;
        }

        // Push final data
        glm::vec3 *vertices = (glm::vec3*)(data + text_data->vertices_offest);
        glm::vec2 *texcoord = (glm::vec2*)(data + text_data->texcoord_offest);
        RGBA      *colors = (RGBA*)(data + text_data->colors_offest);
        u16       *indices = (u16*)(data + text_data->indices_offest);

        Text::PageRange *range = (Text::PageRange*)(data + text_data->ranges_offest);
        memcpy(range, text->page_ranges._data, text->page_ranges._size * sizeof(Text::PageRange));

        u32 tmp, start_indice;
        for (u32 i = 0; i < array::size(text->char_infos); i++)
        {
          const Text::CharInfo &char_info = text->char_infos[i];

          // INDICES
          start_indice = i * 6;
          tmp = i * 4;

          indices[start_indice] = u16(tmp);
          indices[start_indice + 1] = u16(tmp + 1);
          indices[start_indice + 2] = u16(tmp + 2);
          indices[start_indice + 3] = u16(tmp + 2);
          indices[start_indice + 4] = u16(tmp + 3);
          indices[start_indice + 5] = u16(tmp);

          // APPLIES WORLD POSE
          vertices[tmp] = (glm::vec3)(world_pose * glm::vec4(char_info.vertices[0], char_info.vertices[1], 0.f, 1.f));
          vertices[tmp + 1] = (glm::vec3)(world_pose * glm::vec4(char_info.vertices[2], char_info.vertices[3], 0.f, 1.f));
          vertices[tmp + 2] = (glm::vec3)(world_pose * glm::vec4(char_info.vertices[4], char_info.vertices[5], 0.f, 1.f));
          vertices[tmp + 3] = (glm::vec3)(world_pose * glm::vec4(char_info.vertices[6], char_info.vertices[7], 0.f, 1.f));

          // TEX_COORD
          const Char &chr = font_resource::get(text->font, (u64)char_info.chr);
          memcpy(&texcoord[tmp], chr.tex_coord, 4 * TEXCOORD_SIZE);

          // COLORS
          for (u32 i = 0; i < 4; i++) colors[tmp+i] = text->color;
        }
      }
    }

    void draw(const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer)
    {
      Array<GraphicRange> ranges(memory_globals::default_allocator());

      PRINT_GL_LAST_ERROR();

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_DEPTH_TEST);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      for (u32 i = 0; i < num_items; i++)
      {
        const u8 *data = renderer::get_data(graphics + i);
        const TextData &text_data = *(const TextData*)data;
        const u16 *indices = (u16*)(data + text_data.indices_offest);

        // send data to buffers
        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.vertices);
        glBufferData(GL_ARRAY_BUFFER, SPRITE_VERTICES_SIZE * text_data.num_chars, (f32*)(data + text_data.vertices_offest), GL_STATIC_DRAW);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.texcoords);
        glBufferData(GL_ARRAY_BUFFER, SPRITE_TEXCOORD_SIZE * text_data.num_chars, (f32*)(data + text_data.texcoord_offest), GL_STATIC_DRAW);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.colors);
        glBufferData(GL_ARRAY_BUFFER, SPRITE_COLORS_SIZE * text_data.num_chars, (u8*)(data + text_data.colors_offest), GL_STATIC_DRAW);
        PRINT_GL_LAST_ERROR();

        // Compose vertex array buffer
        glBindVertexArray(render_buffer.vao);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.texcoords);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.colors);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (GLubyte*)NULL);
        PRINT_GL_LAST_ERROR();

        // render

        glBindVertexArray(render_buffer.vao);
        PRINT_GL_LAST_ERROR();

        const Text::PageRange *range = (Text::PageRange*)(data + text_data.ranges_offest);
        const Text::PageRange *rend = range + text_data.num_ranges;

        for (; range < rend; range++) {
          const u32 &tex = renderer::get_font(range->font, range->page);
          glBindTexture(GL_TEXTURE_2D, tex);
          glDrawElements(GL_TRIANGLES, 6 * range->num_chars, GL_UNSIGNED_SHORT, indices + range->pad);
          PRINT_GL_LAST_ERROR();
        }
      }

      glBindVertexArray(0);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      PRINT_GL_LAST_ERROR();
    }
    

    Pose &get_pose(TextSystem &system, u64 text)
    {
      return (*idlut::lookup(system, text))->pose;
    }

    void get_size(TextSystem &system, u64 text, glm::vec2 &size)
    {
      Text *t = *idlut::lookup(system, text);
      size = glm::vec2(t->width, t->height);
    }

    void get_width(TextSystem &system, u64 text, f32 &v)
    {
      v = (f32)(*idlut::lookup(system, text))->width;
    }

    void get_height(TextSystem &system, u64 text, f32 &v)
    {
      v = (f32)(*idlut::lookup(system, text))->height;
    }


  
    void set(TextSystem &system, u64 text, const FontResource *font, const char *string, TextAlign align, f32 scale, const Color &color)
    {
      ASSERT(string);

      Allocator &a = *system._data._allocator;
      Text *t = *idlut::lookup(system, text);

      if (t->string)
        a.deallocate(t->string);
      t->string = (char*)a.allocate(sizeof(char)*strlen(string));
      strcpy(t->string, string);

      t->font  = font;
      t->align = align;
      t->scale = scale;
      t->color = color;

      _set_text(t);
    }

    void set_string(TextSystem &system, u64 text, const char *string)
    {
      Allocator &a = *system._data._allocator;
      Text *t = *idlut::lookup(system, text);

      if (t->string){
        a.deallocate(t->string);
        t->string = NULL;
      }

      if (string)
      {
        t->string = (char*)a.allocate(sizeof(char)*strlen(string));
        strcpy(t->string, string);
        _set_text(t);
        return;
      }

      array::clear(t->char_infos);
      array::clear(t->page_ranges);
    }

    void set_font(TextSystem &system, u64 text, const FontResource *font)
    {
      Text *t = *idlut::lookup(system, text);
      t->font = font;
      _set_text(t);
    }

    void set_alignment(TextSystem &system, u64 text, TextAlign align)
    {
      Text *t = *idlut::lookup(system, text);
      t->align = align;
      _set_text(t);
    }

    void set_scale(TextSystem &system, u64 text, f32 scale)
    {
      Text *t = *idlut::lookup(system, text);
      t->scale = scale;
      _set_text(t);
    }

    void set_color(TextSystem &system, u64 text, const Color &color)
    {
      (*idlut::lookup(system, text))->color = color;
    }
  }

  TextSystem::~TextSystem() {
    Allocator &a = *this->_data._allocator;
    const TextSystem::Entry *e, *end = idlut::end(*this);
    for (e = idlut::begin(*this); e < end; e++) {
      a.deallocate(e->value->string);
      MAKE_DELETE(a, Text, e->value);
    }
  }
}