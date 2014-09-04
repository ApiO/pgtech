#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/glm.hpp>
#include <pose.h>

#include <runtime/types.h>
#include <runtime/hash.h>
#include <runtime/trace.h>
#include <runtime/memory.h>
#include <runtime/idlut.h>

#include <renderer/renderer.h>
#include <utils/ogl_debug.h>

//#include <culling/culling_system.h>
#include "sprite_resource.h"
#include "sprite_system.h"

namespace
{
  using namespace pge;

  const Color DEFAULT_SPRITE_COLOR(255, 255, 255, 255);

  struct SpriteData
  {
    glm::vec3 vertices[4];
    glm::vec2 texcoord[4];
    Color     colors[4];
    u32 tex_name;
    u32 region_name;
    u8  blend_mode;
  };

  const u16 INDICES[] ={ 0u, 1u, 2u, 2u, 3u, 0u };
}

namespace pge
{
  namespace sprite_system
  {
    u64 create(SpriteSystem &system, const SpriteResource *resource, u32 group, u32 order)
    {
      Sprite sprite;
      sprite.group = group;
      sprite.order = order;
      sprite.resource = resource;
      sprite.frame = sprite.resource->setup_frame;
      sprite.color = DEFAULT_SPRITE_COLOR;

      sprite.aabb = culling_system::create_aabb(*system.culling_system, (glm::vec2*)sprite_resource::aabb(resource, sprite.frame));

      return idlut::add(system.sprites, sprite);
    }

    void update(SpriteSystem &system)
    {
      glm::mat4 m;
      IdLookupTable<Sprite>::Entry *e, *end = idlut::end(system.sprites);
      bool update_aabb;

      for (e = idlut::begin(system.sprites); e < end; e++) {
        update_aabb = e->value.frame_changed || e->value.pose._dirty;

        if (e->value.pose._dirty)
          pose::update(e->value.pose);

        if (update_aabb) {
          pose::get_world_pose(e->value.pose, m);
          culling_system::update(*system.culling_system, e->value.aabb, m,
                                 (glm::vec2*)sprite_resource::aabb(e->value.resource, e->value.frame));
          e->value.frame_changed = false;
        }
      }
    }

    Pose &get_pose(SpriteSystem &system, u64 sprite)
    {
      return idlut::lookup(system.sprites, sprite)->pose;
    }

    void set_frame(SpriteSystem &system, u64 sprite, i32 frame)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      s.frame = frame;
      s.frame_changed = true;
    }

    void get_color(SpriteSystem &system, u64 sprite, Color &color)
    {
      color = idlut::lookup(system.sprites, sprite)->color;
    }

    void set_color(SpriteSystem &system, u64 sprite, const Color &color)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      s.color = color;
    }

    i32 get_frame(SpriteSystem &system, u64 sprite, const char *name)
    {
      const u32 h = murmur_hash_32(name);
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      const SpriteResource::Frame *f = sprite_resource::frames(s.resource);
      for (i32 i = 0; i < s.resource->num_frames; i++) {
        if (f[i].name == h)
          return i;
      }
      return -1;
    }

    i32 get_setup_frame(SpriteSystem &system, u64 sprite)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      return s.resource->setup_frame;
    }

    i32 get_current_frame(SpriteSystem &system, u64 sprite)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      return s.frame;
    }

    i32 get_num_frames(SpriteSystem &system, u64 sprite)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      return s.resource->num_frames;
    }

    void destroy(SpriteSystem &system, u64 sprite)
    {
      if (!idlut::has(system.sprites, sprite))
        return;

      culling_system::destroy_aabb(*system.culling_system, idlut::lookup(system.sprites, sprite)->aabb);
      idlut::remove(system.sprites, sprite);
    }

    void get_size(SpriteSystem &system, u64 sprite, glm::vec2 &size)
    {
      Sprite &s = *idlut::lookup(system.sprites, sprite);
      const SpriteResource::Frame *res_frame = &sprite_resource::frames(s.resource)[s.frame];
      const f32 *res_vertices = sprite_resource::vertices(s.resource, res_frame);
      size.x = abs(res_vertices[0] - res_vertices[4]);
      size.y = abs(res_vertices[1] - res_vertices[5]);
    }

    void gather(SpriteSystem &system)
    {
      i32 skipped = 0;
      const IdLookupTable<Sprite>::Entry *e, *end = idlut::end(system.sprites);
      for (e = idlut::begin(system.sprites); e < end; e++) {

        if (!culling_system::visible(*system.culling_system, e->value.aabb)) {
          skipped++;
          continue;
        }

        const Sprite *sprite = &e->value;

        glm::mat4 world_pose;
        pose::get_world_pose(sprite->pose, world_pose);

        glm::vec3 translation;
        pose::get_world_translation(sprite->pose, translation);

        SortKey sort_key;
        sort_key.z = translation.z;
        sort_key.value = ((u64)sprite->group << 32) & (u64)sprite->order;

        SpriteData &data = *(SpriteData*)renderer::create_graphic(GRAPHIC_TYPE_SPRITE, sizeof(SpriteData), sort_key);

        const SpriteResource::Frame *res_frame = &sprite_resource::frames(sprite->resource)[sprite->frame];
        const f32 *res_vertices = sprite_resource::vertices(sprite->resource, res_frame);

        data.vertices[0] = glm::vec3(res_vertices[0], res_vertices[1], 0);
        data.vertices[1] = glm::vec3(res_vertices[2], res_vertices[3], 0);
        data.vertices[2] = glm::vec3(res_vertices[4], res_vertices[5], 0);
        data.vertices[3] = glm::vec3(res_vertices[6], res_vertices[7], 0);

        for (i32 i = 0; i < 4; i++)
          data.vertices[i] = (glm::vec3)(world_pose * glm::vec4(data.vertices[i], 1));

        const TextureResource *res_texture = (TextureResource*)resource_manager::get(RESOURCE_TYPE_TEXTURE, res_frame->texture_name);
        const TextureResource::Region *res_region = texture_resource::region(res_texture, res_frame->texture_region);
        const TextureResource::Page *res_page = texture_resource::page(res_texture, res_region);

        if (res_region) {
          i32 stride = res_region->rotated ? 1 : 0;
          i32 region_width = res_region->rotated ? res_region->height : res_region->width;
          i32 region_height = res_region->rotated ? res_region->width : res_region->height;

          data.texcoord[(stride + 0) % 4] = glm::vec2((f32)res_region->x / res_page->width, (f32)(res_region->y + region_height) / (f32)res_page->height);
          data.texcoord[(stride + 1) % 4] = glm::vec2((f32)res_region->x / res_page->width, (f32)res_region->y / (f32)res_page->height);
          data.texcoord[(stride + 2) % 4] = glm::vec2((f32)(res_region->x + region_width) / (f32)res_page->width, (f32)res_region->y / (f32)res_page->height);
          data.texcoord[(stride + 3) % 4] = glm::vec2((f32)(res_region->x + region_width) / (f32)res_page->width, (f32)(res_region->y + region_height) / (f32)res_page->height);
        } else {
          data.texcoord[0] = data.texcoord[1] = data.texcoord[2] = data.texcoord[3] = glm::vec2(0.0, 0.0);
        }

        Color sprite_res_color = *(Color*)&res_frame->color;

        Color color = sprite_res_color != DEFAULT_SPRITE_COLOR ? sprite_res_color : sprite->color;

        memcpy(&data.colors[0], &color, sizeof(Color));
        memcpy(&data.colors[1], &color, sizeof(Color));
        memcpy(&data.colors[2], &color, sizeof(Color));
        memcpy(&data.colors[3], &color, sizeof(Color));

        data.tex_name = res_frame->texture_name;
        data.region_name = res_frame->texture_region;
        data.blend_mode = sprite->resource->blend_mode;
      }
    }

    void draw(const Graphic *graphics, u32 num_graphics, RenderBuffer &render_buffer)
    {
      if (!num_graphics) return;
      Array<GraphicRange> ranges(memory_globals::default_allocator());

      array::resize(render_buffer.vertices, 3 * 4 * num_graphics);
      array::resize(render_buffer.tex_coord, 2 * 4 * num_graphics);
      array::resize(render_buffer.colors, 4 * 4 * num_graphics);
      array::resize(render_buffer.indices, 6 * num_graphics);

      const SpriteData *fisrtSprite = (const SpriteData*)renderer::get_data(graphics);
      GraphicRange range;
      u16 indices_offset = 0;
      range.mode = DRAW_MODE_TRIANGLE;
      range.blend = (BlendMode)fisrtSprite->blend_mode;
      range.texture = renderer::get_texture(fisrtSprite->tex_name, fisrtSprite->region_name);
      range.indice_offset = 0;
      range.num_indices = 0;

      for (u32 i = 0; i < num_graphics; i++) {
        const SpriteData *sprite = (const SpriteData*)renderer::get_data(graphics + i);
        memcpy(array::begin(render_buffer.vertices) + (i * 12), sprite->vertices, SPRITE_VERTICES_SIZE);
        memcpy(array::begin(render_buffer.tex_coord) + (i * 8), sprite->texcoord, SPRITE_TEXCOORD_SIZE);
        memcpy(array::begin(render_buffer.colors) + (i * 16), sprite->colors, SPRITE_COLORS_SIZE);

        const u32 texture = renderer::get_texture(sprite->tex_name, sprite->region_name);
        if (texture != range.texture || sprite->blend_mode != range.blend) {
          array::push_back(ranges, range);
          range.texture = texture;
          range.blend   = (BlendMode)sprite->blend_mode;
          range.indice_offset = indices_offset;
          range.num_indices = 0;
        }

        { // create indices
          const u32 ob = i * 6;
          const u16 oi = (u16)i * 4;
          render_buffer.indices[ob + 0] = oi + 0u;
          render_buffer.indices[ob + 1] = oi + 1u;
          render_buffer.indices[ob + 2] = oi + 2u;
          render_buffer.indices[ob + 3] = oi + 2u;
          render_buffer.indices[ob + 4] = oi + 3u;
          render_buffer.indices[ob + 5] = oi + 0u;
        }

        range.num_indices += 6;
        indices_offset += 6;
        if (i == num_graphics - 1)
          array::push_back(ranges, range);
      }

      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.vertices);
      glBufferData(GL_ARRAY_BUFFER, SPRITE_VERTICES_SIZE * num_graphics, array::begin(render_buffer.vertices), GL_STATIC_DRAW);
      PRINT_GL_LAST_ERROR();

      glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.texcoords);
      glBufferData(GL_ARRAY_BUFFER, SPRITE_TEXCOORD_SIZE * num_graphics, array::begin(render_buffer.tex_coord), GL_STATIC_DRAW);
      PRINT_GL_LAST_ERROR();

      glBindBuffer(GL_ARRAY_BUFFER, render_buffer.vbo.buf.colors);
      glBufferData(GL_ARRAY_BUFFER, SPRITE_COLORS_SIZE * num_graphics, array::begin(render_buffer.colors), GL_STATIC_DRAW);
      PRINT_GL_LAST_ERROR();

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

      for (u32 i = 0; i < array::size(ranges); i++) {
        switch (ranges[i].blend) {
          case BLEND_MODE_MULTIPLY:
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
            break;
          case BLEND_MODE_SCREEN:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
            break;
          case BLEND_MODE_OVERLAY:
            glBlendFunc(GL_DST_COLOR, GL_ONE);
            break;
          case BLEND_MODE_ADDITIVE:
            glBlendFunc(GL_ONE, GL_ONE);
            break;
          case BLEND_MODE_NORMAL:
          default:
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }

        glBindTexture(GL_TEXTURE_2D, ranges[i].texture);
        glDrawElements(GL_TRIANGLES, ranges[i].num_indices, GL_UNSIGNED_SHORT, array::begin(render_buffer.indices) + ranges[i].indice_offset);
      }

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBindTexture(GL_TEXTURE_2D, 0);
      glBindVertexArray(0);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      PRINT_GL_LAST_ERROR();
    }
  }
}