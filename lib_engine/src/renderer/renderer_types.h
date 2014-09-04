#pragma once

#include <runtime/collection_types.h>
#include <glm/glm.hpp>
#include <data/types.h>

namespace pge
{
  typedef glm::u8vec4 RGBA;

  enum GraphicType
  {
    GRAPHIC_TYPE_SPRITE = 0,
    GRAPHIC_TYPE_TEXT,
    GRAPHIC_TYPE_PRIMITIVE,
    GRAPHIC_TYPE_PARTICLE
  };

  enum DrawMode
  {
    DRAW_MODE_LINE = 0,
    DRAW_MODE_TRIANGLE
  };

  struct GraphicRange
  {
    GraphicRange(DrawMode m, u16 num_indices, u16 index_offset);
    GraphicRange() {}
    DrawMode mode;
    BlendMode blend;
    u16 num_indices;
    u16 indice_offset;
    u32 texture;
  };

  struct SortKey
  {
    f32 z;
    u64 value;
  };

  struct Graphic
  {
    GraphicType type;
    u32         data_offset;
    SortKey     sort_key;
  };

  struct RenderBuffer
  {
    RenderBuffer(Allocator &a);
    Array<f32> vertices;
    Array<f32> tex_coord;
    Array<u8>  colors;
    Array<u16> indices;
    u32 vao;
    union
    {
      u32 buffers[5];
      struct {
        u32 vertices, texcoords, colors, indices;
      }buf;
    } vbo;
  };
}