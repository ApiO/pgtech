#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/glm.hpp>

#include <runtime/types.h>
#include <runtime/assert.h>
#include <runtime/memory.h>
#include <runtime/idlut.h>

//#include <culling/culling_system.h>
#include <renderer/renderer_types.h>
#include <renderer/renderer.h>
#include <utils/ogl_debug.h>
#include "geometric_system.h"

#define NUM_INDICES_MAX 65536

namespace
{
  using namespace pge;

  const u64 SORT_KEY_VALUE = 0u;

  enum PrimitiveType
  {
    PRIMITIVE_TYPE_CIRCLE,
    PRIMITIVE_TYPE_CHAIN,
    PRIMITIVE_TYPE_POLYGON,
    PRIMITIVE_TYPE_BOX
  };

  struct PrimitiveData
  {
    PrimitiveData(PrimitiveType type, DrawMode mode, pge::u32 num_vertices, const Color &color);
    PrimitiveType type;
    DrawMode      mode;
    pge::u32      num_vertices;
    Color         color;
    glm::mat4     world_pose;
  };

  struct CircleData : PrimitiveData
  {
    CircleData(const glm::vec3 &center, f32 radius, const Color &color, bool surface);
    glm::vec3 center;
    pge::f32  radius;
    bool      surface;
  };

  struct ChainData : PrimitiveData
  {
    ChainData(u32 num_vertices, const Color &color);
    pge::u16 vertices_offest;
  };

  struct PolygonData : PrimitiveData
  {
    PolygonData(pge::u32 num_vertices, const Color &color);
    pge::u16 vertices_offest;
  };

  struct BoxData : PrimitiveData
  {
    BoxData(bool surface);
    pge::u16 vertices_offest;
    pge::u16 colors_offset;
    bool     surface;
  };

  inline PrimitiveData::PrimitiveData(PrimitiveType t, DrawMode m, u32 n, const Color &c)
    : type(t), mode(m), color(c), num_vertices(n), world_pose(1.f) {}

  inline CircleData::CircleData(const glm::vec3 &_center, f32 _radius, const Color &_color, bool s)
    : PrimitiveData(PRIMITIVE_TYPE_CIRCLE, s ? DRAW_MODE_TRIANGLE : DRAW_MODE_LINE, ((u16)(4 * sqrtf(_radius)) + (s ? 1 : 0)), _color),
    center(_center),
    radius(_radius),
    surface(s){}

  inline ChainData::ChainData(u32 n, const Color &c)
    : PrimitiveData(PRIMITIVE_TYPE_CHAIN, DRAW_MODE_LINE, n, c),
    vertices_offest(sizeof(ChainData)){}

  inline PolygonData::PolygonData(u32 n, const Color &c)
    : PrimitiveData(PRIMITIVE_TYPE_POLYGON, DRAW_MODE_LINE, n, c),
    vertices_offest(sizeof(PolygonData)){}

  inline BoxData::BoxData(bool s)
    : PrimitiveData(PRIMITIVE_TYPE_BOX, s ? DRAW_MODE_TRIANGLE : DRAW_MODE_LINE, 4, Color()),
    vertices_offest(sizeof(BoxData)), colors_offset(sizeof(BoxData) + VERTICE_SIZE*4), surface(s){}


  inline u16 get_num_indices(PrimitiveData &data)
  {
    u16 num_indices = 0;
    switch (data.type)
    {
      case PRIMITIVE_TYPE_CIRCLE:
        num_indices = u16(data.mode == DRAW_MODE_TRIANGLE
                          ? (data.num_vertices - 1) * 3
                          : data.num_vertices * 2);
        break;
      case PRIMITIVE_TYPE_CHAIN:
        num_indices = u16((data.num_vertices - 1) * 2);
        break;
      case PRIMITIVE_TYPE_POLYGON:
        num_indices = u16(data.num_vertices * 2);
        break;
      case PRIMITIVE_TYPE_BOX:
        num_indices = u16(data.mode == DRAW_MODE_TRIANGLE ? 6 : 8);
        break;
      default:
        XERROR("Primmitive data type \"%d\" not handled. Missing definition.", data.type);
        break;
    }
    return num_indices;
  }


  inline void calculate_circle_vertices(glm::vec3 *vertices, glm::vec3 center, f32 r, u16 num_segments, glm::mat4 &p)
  {
    //http://slabode.exofire.net/circle_draw.shtml
    f32 theta = (f32)(2 * M_PI / num_segments);
    //precalculate the sine and cosine
    f32 c = cosf(theta), s = sinf(theta);
    f32 t;

    //we start at angle = 0 
    f32 x = r, y = 0.f;

    for (u16 i = 0; i < num_segments; i++)
    {
      vertices[i] = glm::vec3(p * glm::vec4(x + center.x, y + center.y, center.z, 1.f));

      //apply the rotation matrix
      t = x;
      x = c * x - s * y;
      y = s * t + c * y;
    }
  }

  inline void generate_default_colors(RGBA *colors, u16 num_points, RGBA color)
  {
    for (u32 i = 0; i < num_points; i++)
      colors[i] = color;
  }


  inline Graphic *initialize_batch(RenderBuffer &render_buffer, Array<GraphicRange> &ranges, u32 &total_vertices, const Graphic *graphics, u32 &num_graphics)
  {
    const Graphic *start_graphic = graphics;
    PrimitiveData *start_data    = (PrimitiveData *)renderer::get_data(graphics);
    const Graphic *cur_graphic   = start_graphic;
    PrimitiveData *cur_data      = start_data;

    u16 total_indices = 0u;
    u16 num_indices;
    u32 num_vertices;
    GraphicRange *cur_range = NULL;

    num_vertices = start_data->num_vertices;
    num_indices  = get_num_indices(*start_data);
    {
      GraphicRange range(start_data->mode, num_indices, 0);
      cur_range = &array::push_back(ranges, range);
    }
    total_vertices = num_vertices;
    total_indices  = num_indices;

    cur_graphic++;
    cur_data = (PrimitiveData *)renderer::get_data(cur_graphic);


    for (u32 i = 1; i < num_graphics; i++)
    {
      num_vertices = cur_data->num_vertices;
      num_indices  = get_num_indices(*cur_data);

      if ((total_indices + num_indices) > NUM_INDICES_MAX)
      {
        num_graphics -= i;

        // reserves memory
        array::reserve(render_buffer.vertices, total_vertices * 3);
        array::reserve(render_buffer.colors, total_vertices * 4);
        array::reserve(render_buffer.indices, total_indices);

        return (Graphic*)cur_graphic;
      }

      if (start_data->mode != cur_data->mode)
      {
        // save range
        GraphicRange range(cur_data->mode, num_indices, total_indices);
        cur_range = &array::push_back(ranges, range);

        start_data    = cur_data;
        start_graphic = cur_graphic;
      }
      else
      {
        cur_range->num_indices += num_indices;
      }
      total_vertices += num_vertices;
      total_indices  += num_indices;

      cur_graphic++;
      cur_data = (PrimitiveData *)renderer::get_data(cur_graphic);
    }

    // reserves memory
    array::reserve(render_buffer.vertices, total_vertices * 3);
    array::reserve(render_buffer.colors, total_vertices * 4);
    array::reserve(render_buffer.indices, total_indices);

    num_graphics = 0;
    return NULL;
  }

  inline void generate_batch_data(RenderBuffer &render_buffer, const Graphic *graphics, u32 num_graphics)
  {
    glm::vec3 *vertices = (glm::vec3*)array::begin(render_buffer.vertices);
    RGBA  *colors       = (RGBA*)array::begin(render_buffer.colors);
    u16 *indices        = array::begin(render_buffer.indices);

    CircleData *cd = NULL;
    glm::vec3  *tmp_vertices = NULL;
    u16 num_indices, vertices_offest;
    u16 index_buffer_value_offset = 0;

    for (u32 i = 0; i < num_graphics; i++)
    {
      PrimitiveData *gd = (PrimitiveData *)renderer::get_data(graphics + i);
      switch (gd->type)
      {
        case PRIMITIVE_TYPE_CIRCLE:
          cd   = (CircleData*)gd;
          if (gd->mode == DRAW_MODE_TRIANGLE)
          {
            // INDICES
            for (u16 j = 0; j < gd->num_vertices - 1; j++) {
              indices[0] = index_buffer_value_offset;
              indices[1] = index_buffer_value_offset + j + 1;
              indices[2] = index_buffer_value_offset +(j == gd->num_vertices - 2 ? 1 : j + 2);
              indices += 3;
            }
            // VERTICES
            *vertices = glm::vec3(cd->world_pose * glm::vec4(cd->center, 1.f));
            vertices++;

            calculate_circle_vertices(vertices, cd->center, cd->radius, u16(gd->num_vertices - 1), cd->world_pose);
            vertices += gd->num_vertices - 1;
          }
          else
          {
            // INDICES
            for (u16 j = 0; j < gd->num_vertices; j++) {
              indices[0] = index_buffer_value_offset + j;
              indices[1] = index_buffer_value_offset + (j == gd->num_vertices - 1 ? 0 : j + 1);
              indices += 2;
            }

            // VERTICES
            calculate_circle_vertices(vertices, cd->center, cd->radius, (u16)gd->num_vertices, cd->world_pose);
            vertices += gd->num_vertices;
          }
          index_buffer_value_offset += (u16)gd->num_vertices;
          //*/
          break;
        case PRIMITIVE_TYPE_BOX:
          if (gd->mode == DRAW_MODE_TRIANGLE) {
            indices[0] = 0; indices[1] = 1; indices[2] = 3;
            indices[3] = 3; indices[4] = 1; indices[5] = 2;
          } else {
            indices[0] = 0; indices[1] = 1;
            indices[2] = 1; indices[3] = 2;
            indices[4] = 2; indices[5] = 3;
            indices[6] = 3; indices[7] = 0;
          }
          tmp_vertices = (glm::vec3*)((u8*)gd + ((BoxData*)gd)->vertices_offest);
          for (u32 j = 0; j < 4; j++)
            vertices[j] = glm::vec3(gd->world_pose * glm::vec4(tmp_vertices[j], 1.f));
           vertices += gd->num_vertices;
          break;
        case PRIMITIVE_TYPE_CHAIN:
        case PRIMITIVE_TYPE_POLYGON:
          num_indices     = get_num_indices(*gd);
          vertices_offest = gd->type == PRIMITIVE_TYPE_POLYGON
            ? ((PolygonData*)gd)->vertices_offest
            : ((ChainData*)gd)->vertices_offest;

          // VERTICES
          tmp_vertices = (glm::vec3*)((u8*)gd + vertices_offest);
          for (u32 j = 0; j < gd->num_vertices; j++)
            vertices[j] = glm::vec3(gd->world_pose * glm::vec4(tmp_vertices[j], 1.f));
          vertices += gd->num_vertices;

          // INDICES
          if (gd->type == PRIMITIVE_TYPE_POLYGON)
          {
            u16 num_segments = u16(gd->num_vertices == 2 ? 1 : gd->num_vertices);
            for (u16 j = 0; j < num_segments; j++)
            {
              indices[j * 2]       = index_buffer_value_offset + j;
              indices[(j * 2) + 1] = index_buffer_value_offset + (j == num_segments - 1 ? 0 : j + 1);
            }
          }
          else
            for (u16 j = 0; j < gd->num_vertices-1; j++)
            {
              indices[j * 2]       = index_buffer_value_offset + j;
              indices[(j * 2) + 1] = index_buffer_value_offset + j + 1;
            }

          indices += num_indices;
          index_buffer_value_offset += (u16)gd->num_vertices;
          break;
        default:
          XERROR("Primmitive type data\"%d\" not handled. Missing definition.", gd->type);
      }
      // COLORS
      if (gd->type == PRIMITIVE_TYPE_BOX) {
        // todo gérer plusieurs couleurs
        Color *box_colors = (Color*)((u8*)gd + ((BoxData*)gd)->colors_offset);
        memcpy(colors, box_colors, sizeof(Color) * 4);
      } else {
        generate_default_colors(colors, (u16)gd->num_vertices, gd->color);
      }
      colors += gd->num_vertices;
    }
  }

  inline void draw_batch(RenderBuffer &render_buffer, Array<GraphicRange> &ranges, u32 total_vertices)
  {
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    PRINT_GL_LAST_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.vertices);
    glBufferData(GL_ARRAY_BUFFER, VERTICE_SIZE * total_vertices, array::begin(render_buffer.vertices), GL_STATIC_DRAW);
    PRINT_GL_LAST_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.colors);
    glBufferData(GL_ARRAY_BUFFER, COLOR_SIZE * total_vertices, array::begin(render_buffer.colors), GL_STATIC_DRAW);
    PRINT_GL_LAST_ERROR();

    glBindVertexArray(render_buffer.vao);
    PRINT_GL_LAST_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
    PRINT_GL_LAST_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.colors);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (GLubyte*)NULL);
    PRINT_GL_LAST_ERROR();

    //OUTPUT("total_vertices:%u", total_vertices);

    const GraphicRange *range = array::begin(ranges),
      *rend = array::end(ranges);
    u16 *indices= array::begin(render_buffer.indices);
    for (; range < rend; range++)
    {
      GLenum mode;
      switch (range->mode)
      {
        case DRAW_MODE_LINE:      mode = GL_LINES;     break;
        case DRAW_MODE_TRIANGLE:  mode = GL_TRIANGLES; break;
        default:
          XERROR("GL enum \"%d\" not handled. Missing definition.", range->mode);
          return;
      }

      glDrawElements(mode, range->num_indices, GL_UNSIGNED_SHORT, indices + range->indice_offset);
      PRINT_GL_LAST_ERROR();
      //OUTPUT("indices : %u", range->num_indices);
    }
    //OUTPUT("--------------------------");

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    PRINT_GL_LAST_ERROR();
  }
}

namespace pge
{

  GeometricSystem::GeometricSystem(Allocator &a) : IdLookupTable<Geometry>(a){}

  GeometricSystem::~GeometricSystem()
  {
    GeometricSystem::Entry *e, *end = idlut::end(*this);
    for (e = idlut::begin(*this); e < end; e++)
      geometric_system::destroy(*this, e->id);
  }


  namespace geometric_system
  {
    u64 create_line(GeometricSystem &s, const glm::vec3 &a, const glm::vec3 &b, const Color &color)
    {
      Geometry geometry;
      geometry.size = sizeof(ChainData)+VERTICE_SIZE * 2;
      geometry.data = (u8*)(s._data._allocator)->allocate(geometry.size);

      ChainData &prim = *(ChainData*)geometry.data;
      prim = ChainData(2, color);

      glm::vec3 *vertices = (glm::vec3*)(geometry.data + prim.vertices_offest);
      vertices[0] = a;
      vertices[1] = b;

      return idlut::add(s, geometry);
    }

    u64 create_chain(GeometricSystem &s, const glm::vec3 *vertices, u32 num_vertices, const Color &color)
    {
      Geometry geometry;
      geometry.size = sizeof(ChainData)+VERTICE_SIZE * num_vertices;
      geometry.data = (u8*)(s._data._allocator)->allocate(geometry.size);

      ChainData &prim = *(ChainData*)geometry.data;
      prim = ChainData(num_vertices, color);

      glm::vec3 *v = (glm::vec3*)(geometry.data + prim.vertices_offest);
      memcpy(v, vertices, VERTICE_SIZE * num_vertices);

      return idlut::add(s, geometry);
    }

    u64 create_polygon(GeometricSystem &s, const glm::vec3 *vertices, u32 num_vertices, const Color &color)
    {
      Geometry geometry;
      geometry.size = sizeof(PolygonData)+VERTICE_SIZE * num_vertices;
      geometry.data = (u8*)(s._data._allocator)->allocate(geometry.size);

      PolygonData &prim = *(PolygonData*)geometry.data;
      prim = PolygonData(num_vertices, color);

      glm::vec3 *v = (glm::vec3*)(geometry.data + prim.vertices_offest);
      memcpy(v, vertices, VERTICE_SIZE * num_vertices);

      return idlut::add(s, geometry);
    }

    u64 create_box(GeometricSystem &s, f32 width, f32 height, const Color *colors, bool surface)
    {
      Geometry  geometry;
      glm::vec3 vertices[4];
      geometry.size = sizeof(BoxData)+(VERTICE_SIZE * 4) + (sizeof(Color) * 4);
      geometry.data = (u8*)(s._data._allocator)->allocate(geometry.size);

      BoxData &prim = *(BoxData*)geometry.data;
      prim = BoxData(surface);

      glm::vec3 *v = (glm::vec3*)(geometry.data + prim.vertices_offest);
      vertices[0] = glm::vec3(-width/2,  height/2, 0);
      vertices[1] = glm::vec3(-width/2, -height/2, 0);
      vertices[2] = glm::vec3( width/2, -height/2, 0);
      vertices[3] = glm::vec3( width/2,  height/2, 0);
      memcpy(v, vertices, VERTICE_SIZE * 4);

      Color *c = (Color*)(geometry.data + prim.colors_offset);
      memcpy(c, colors, 4 * sizeof(Color));

      return idlut::add(s, geometry);
    }

    u64 create_circle(GeometricSystem &s, const glm::vec3 &center, f32 r, const Color &color, bool surface)
    {
      Geometry geometry;
      geometry.size = sizeof(CircleData);
      geometry.data = (u8*)(s._data._allocator)->allocate(geometry.size);

      CircleData &prim = *(CircleData*)geometry.data;
      prim = CircleData(center, r, color, surface);

      return idlut::add(s, geometry);
    }


    void gather_line(const glm::vec3 &a, const glm::vec3 &b, const Color &color)
    {
      u32 data_size = sizeof(ChainData)+VERTICE_SIZE * 2;

      SortKey sortkey;
      sortkey.z     = a.z;
      sortkey.value = DRAW_MODE_LINE;

      u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, data_size, sortkey);
      ChainData &prim = *(ChainData*)data;
      prim = ChainData(2, color);

      glm::vec3 *vertices = (glm::vec3*)(data + prim.vertices_offest);
      vertices[0] = a;
      vertices[1] = b;
    }

    void gather_chain(const glm::vec3 *vertices, u32 num_vertices, const Color &color)
    {
      u32 data_size = sizeof(ChainData)+VERTICE_SIZE * num_vertices;

      SortKey sortkey;
      sortkey.z     = vertices->z;
      sortkey.value = DRAW_MODE_LINE;

      u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, data_size, sortkey);
      ChainData &prim = *(ChainData*)data;
      prim = ChainData(num_vertices, color);

      glm::vec3 *v = (glm::vec3*)(data + prim.vertices_offest);
      memcpy(v, vertices, VERTICE_SIZE * num_vertices);
    }

    void gather_polygon(const glm::vec3 *vertices, u32 num_vertices, const Color &color)
    {
      u32 data_size = sizeof(PolygonData)+VERTICE_SIZE * num_vertices;

      SortKey sortkey;
      sortkey.z     = vertices->z;
      sortkey.value = DRAW_MODE_LINE;

      u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, data_size, sortkey);
      PolygonData &prim = *(PolygonData*)data;
      prim = PolygonData(num_vertices, color);

      glm::vec3 *v = (glm::vec3*)(data + prim.vertices_offest);
      memcpy(v, vertices, VERTICE_SIZE * num_vertices);
    }

    void gather_box(f32 width, f32 height, const Color *colors, bool surface)
    {
      glm::vec3 vertices[4];
      SortKey sortkey;
      sortkey.z     = vertices->z;
      sortkey.value = surface ? DRAW_MODE_TRIANGLE : DRAW_MODE_LINE;

      u32 data_size = sizeof(BoxData)+(VERTICE_SIZE * 4) + (sizeof(Color) * 4);
      u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, data_size, sortkey);

      BoxData &prim = *(BoxData*)data;
      prim = BoxData(surface);

      glm::vec3 *v = (glm::vec3*)(data + prim.vertices_offest);
      vertices[0] = glm::vec3(-width/2,  height/2, 0);
      vertices[1] = glm::vec3(-width/2, -height/2, 0);
      vertices[2] = glm::vec3( width/2, -height/2, 0);
      vertices[3] = glm::vec3( width/2,  height/2, 0);
      memcpy(v, vertices, VERTICE_SIZE * 4);

      Color *c = (Color*)(data + prim.colors_offset);
      memcpy(c, colors, 4 * sizeof(Color));
    }

    void gather_circle(const glm::vec3 &center, f32 r, const Color &color, bool surface)
    {
      SortKey sortkey;
      sortkey.z     = center.z;
      sortkey.value = surface ? DRAW_MODE_TRIANGLE : DRAW_MODE_LINE;

      u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, sizeof(CircleData), sortkey);
      CircleData &prim = *(CircleData*)data;
      prim = CircleData(center, r, color, surface);
    }


    Pose &get_pose(GeometricSystem &s, u64 geometry)
    {
      return idlut::lookup(s, geometry)->pose;
    }

    void update(GeometricSystem &s)
    {
      GeometricSystem::Entry *e, *end = idlut::end(s);
      for (e = idlut::begin(s); e < end; e++){
        pose::update(e->value.pose);
        pose::get_world_pose(e->value.pose, ((PrimitiveData*)e->value.data)->world_pose);
      }
    }

    void gather(const GeometricSystem &s)
    {
      glm::mat4 world_pose;
      SortKey   sortkey;

      const GeometricSystem::Entry *e, *end = idlut::end(s);
      for (e = idlut::begin(s); e < end; e++)
      {
        sortkey.value = ((PrimitiveData*)e->value.data)->mode;

        switch (((PrimitiveData*)e->value.data)->type)
        {
          case PRIMITIVE_TYPE_CIRCLE:
            sortkey.z = ((CircleData*)e->value.data)->center.z;
            break;
          case PRIMITIVE_TYPE_CHAIN:
            sortkey.z = ((glm::vec3*)(e->value.data + ((ChainData *)e->value.data)->vertices_offest))->z;
            break;
          case PRIMITIVE_TYPE_POLYGON:
            sortkey.z = ((glm::vec3*)(e->value.data + ((PolygonData *)e->value.data)->vertices_offest))->z;
            break;
          case PRIMITIVE_TYPE_BOX:
            sortkey.z = ((glm::vec3*)(e->value.data + ((BoxData*)e->value.data)->vertices_offest))->z;
            break;
        }

        u8 *data = renderer::create_graphic(GRAPHIC_TYPE_PRIMITIVE, e->value.size, sortkey);
        memcpy(data, e->value.data, e->value.size);
      }
    }

    void draw(const Graphic *graphics, u32 num_graphics, RenderBuffer &render_buffer)
    {
      Array<GraphicRange> ranges(memory_globals::default_allocator());

      const Graphic *next_graphic   = graphics;
      const Graphic *batch_graphics = NULL;

      u32 num_items = num_graphics;

      //i32 num_batch = 0, num_range = 0;
      while (next_graphic)
      {
        u32 batch_num_vertices = 0;
        u32 batch_num_graphics = num_items;

        array::clear(ranges);
        array::clear(render_buffer.vertices);
        array::clear(render_buffer.colors);
        array::clear(render_buffer.indices);

        batch_graphics = next_graphic;
        next_graphic = initialize_batch(render_buffer, ranges, batch_num_vertices, next_graphic, num_items);

        batch_num_graphics -= num_items;
        generate_batch_data(render_buffer, batch_graphics, batch_num_graphics);

        draw_batch(render_buffer, ranges, batch_num_vertices);
        //num_range += ranges._size;
        //num_batch++;
      }
      //printf("    geometric_system :: num_batch = %d\tnum_range = %d\n", num_batch, num_range);

    }

    void destroy(GeometricSystem &s, u64 geometry)
    {
      (s._data._allocator)->deallocate(idlut::lookup(s, geometry)->data);
      idlut::remove(s, geometry);
    }
  }
}