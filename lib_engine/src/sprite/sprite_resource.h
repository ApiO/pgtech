#pragma once

#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <runtime/memory.h>
#include <runtime/array.h>
#include <data/sprite.h>
#include <data/texture.h>
#include <resource/texture_resource.h>
#include <resource/resource_manager.h>

namespace
{
  using namespace pge;

    struct FrameAABB
    {
      glm::vec2 min;
      glm::vec2 max;
    };

  const u32 AABB_SIZE = sizeof(Array<FrameAABB>);
  const u32 ARRAY_SIZE = sizeof(Array<f32>);

  inline f32 min(f32 a, f32 b) { return ((a) < (b)) ? (a) : (b); }
  inline f32 max(f32 a, f32 b) { return ((a) > (b)) ? (a) : (b); }

  inline void calculate_aabb(FrameAABB &aabb, glm::vec2 &a, glm::vec2 &b, glm::vec2 &c, glm::vec2 &d)
  {
    aabb.min.x = min(min(a.x, b.x), min(c.x, d.x));
    aabb.min.y = min(min(a.y, b.y), min(c.y, d.y));
    aabb.max.x = max(max(a.x, b.x), max(c.x, d.x));
    aabb.max.y = max(max(a.y, b.y), max(c.y, d.y));
  }
}

namespace pge
{
  namespace sprite_resource
  {
    void  register_type(void);
    void *load(FILE *file, u32 name, u32 size);
    void  patch_up(void *data, u32 name);
    void  unload(void *data, u32 name);

    const SpriteResource::Frame *frames(const SpriteResource *sprite);
    const f32 *vertices(const SpriteResource *sprite, const SpriteResource::Frame *frame);
    const glm::vec2 *aabb(const SpriteResource *sprite, u32 frame);
  }

  namespace sprite_resource
  {
    inline void register_type(void)
    {
      resource_manager::register_type(RESOURCE_TYPE_SPRITE,
        &sprite_resource::load,
        &resource_manager::default_bring_in,
        &sprite_resource::patch_up,
        &resource_manager::default_bring_out,
        &sprite_resource::unload);
    }

    inline void *load(FILE *file, u32 name, u32 size)
    {
      (void)name;

      void *resource = memory_globals::default_allocator().allocate(ARRAY_SIZE + AABB_SIZE + size);
      fread((u8*)resource + ARRAY_SIZE + AABB_SIZE, size, 1, file);

      new (resource)Array<f32>(memory_globals::default_allocator());
      new ((void*)((u8*)resource + ARRAY_SIZE))Array<FrameAABB>(memory_globals::default_allocator());

      return (u8*)resource + ARRAY_SIZE + AABB_SIZE;
    }

    inline void patch_up(void *data, u32 name)
    {
      (void)name;
      const SpriteResource *sprite = (SpriteResource*)data;

      Array<f32> &vertices = *(Array<f32>*)((u8*)data - ARRAY_SIZE - AABB_SIZE);
      Array<FrameAABB> &aabb = *(Array<FrameAABB>*)((u8*)data - AABB_SIZE);

      array::resize(vertices, sprite->num_frames * 8);
      array::resize(aabb, sprite->num_frames);
      
      for (u16 i = 0; i < sprite->num_frames; i++) {
        const SpriteResource::Frame *frame = &sprite_resource::frames(sprite)[i];
        const TextureResource *texture = (TextureResource*)resource_manager::get(RESOURCE_TYPE_TEXTURE, frame->texture_name);

        // TODO : retourner une région par défaut pour les régions vides
        const TextureResource::Region *region = texture_resource::region(texture, frame->texture_region);

        const u32 offset = i * 8;

        if (!region) {
          vertices[offset + 0] = 0;
          vertices[offset + 1] = 0;
          vertices[offset + 2] = 0;
          vertices[offset + 3] = 0;

          aabb[i].min = glm::vec2(0);
          aabb[i].max = glm::vec2(0);
        }
        else{
          const f32 original_half_w = (region->margin[3] + region->width + region->margin[1]) * .5f;
          const f32 original_half_h = (region->margin[0] + region->height + region->margin[2]) * .5f;

          const f32 half_w = region->width * .5f;
          const f32 half_h = region->height * .5f;

          const f32 center_x = -original_half_w + region->margin[3] + half_w;
          const f32 center_y = +original_half_h - region->margin[0] - half_h;

          const glm::vec3 zangle(0, 0, 1);
          const glm::vec3 t(frame->texture_pose.tx, frame->texture_pose.ty, 0.f);
          const glm::quat r = glm::quat(glm::vec3(frame->texture_pose.rx, frame->texture_pose.ry, frame->texture_pose.rz));
          const glm::vec3 s(frame->texture_pose.sx, frame->texture_pose.sy, 1.f);

          glm::mat4 trs = glm::translate(glm::mat4(1.f), t);
          trs = trs * glm::toMat4(r);
          trs = glm::scale(trs, s);
          glm::vec4 tmp_vert;

          tmp_vert = trs * glm::vec4(center_x - half_w, center_y + half_h, 0.f, 1.f);
          vertices[offset + 0] = tmp_vert.x;
          vertices[offset + 1] = tmp_vert.y;

          tmp_vert = trs * glm::vec4(center_x - half_w, center_y - half_h, 0.f, 1.f);
          vertices[offset + 2] = tmp_vert.x;
          vertices[offset + 3] = tmp_vert.y;

          tmp_vert = trs * glm::vec4(center_x + half_w, center_y - half_h, 0.f, 1.f);
          vertices[offset + 4] = tmp_vert.x;
          vertices[offset + 5] = tmp_vert.y;

          tmp_vert = trs * glm::vec4(center_x + half_w, center_y + half_h, 0.f, 1.f);
          vertices[offset + 6] = tmp_vert.x;
          vertices[offset + 7] = tmp_vert.y;

          calculate_aabb(aabb[i],
            *(glm::vec2*)(vertices._data + offset),
            *(glm::vec2*)(vertices._data + offset + 2),
            *(glm::vec2*)(vertices._data + offset + 4),
            *(glm::vec2*)(vertices._data + offset + 6));
        }
      }
    }

    inline void unload(void *data, u32 name)
    {
      (void)name;

      Array<f32> *vertices = (Array<f32>*)((u8*)data - ARRAY_SIZE - AABB_SIZE);
      vertices->~Array();

      ((Array<FrameAABB>*)((u8*)data - AABB_SIZE))->~Array();

      memory_globals::default_allocator().deallocate(vertices);
    }

    inline const SpriteResource::Frame *frames(const SpriteResource *sprite)
    {
      return (SpriteResource::Frame*)((u8*)sprite + sizeof(SpriteResource));
    }

    inline const glm::vec2 *aabb(const SpriteResource *sprite, u32 frame)
    {
      return (glm::vec2 *)array::begin(*(Array<FrameAABB>*)((u8*)sprite - AABB_SIZE)) + frame * 2;
    }

    inline const f32 *vertices(const SpriteResource *sprite, const SpriteResource::Frame *frame)
    {
      return array::begin(*(Array<f32>*)((u8*)sprite - ARRAY_SIZE - AABB_SIZE)) + (frame - frames(sprite)) * 8;
    }
  }
}