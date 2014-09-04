#pragma once

#include <glm/glm.hpp>
#include <runtime/types.h>
#include <runtime/collection_types.h>

#include <types.h>
#include "compiler_types.h"

namespace pge
{
  namespace compiler
  {
    void normalize_path(char *str);
    void write_json_vec2(FILE *stream, const Json &jsn, u64 id);
    void write_json_vec3(FILE *stream, const Json &jsn, u64 id);
    void write_json_vec4(FILE *stream, const Json &jsn, u64 id);
    void write_json_mat4(FILE *stream, const Json &jsn, u64 id);
    void read_json_pose(const Json &jsn, u64 id, PoseResource &p);
    void write_json_pose(FILE *stream, const Json &jsn, u64 id);
    void read_json_color(const Json &jsn, u64 id, u8* c, const char *attribute);
    void read_json_texture(Work &w, const Json &jsn, u64 id, u32 *name, u32 *region);

    bool load_dependency(Work &w, ResourceType type, const char *name, string_stream::Buffer &buf);
    bool load_dependency(Work &w, ResourceType type, const char *name, Json &jsn);
    u32  create_reference(Work &w, ResourceType type, const char *name);
    u32  create_id_string(Work &w, const char *res_name);
    bool load_bytes(const char *path, string_stream::Buffer &buf);
    bool load_json(Work &w, const char *path, ResourceType type, Json &jsn);
  }

  namespace compiler
  {
    inline void normalize_path(char *str)
    {
      const char *start = str,
        *end   = str + strlen(str);
      while (start < end) {
        *str++ = (*start == '\\') ? '/' : *start;
        start++;
      }
    }

    inline void write_json_vec2(FILE *stream, const Json &jsn, u64 id)
    {
      f32 tmp;
      tmp = (f32)json::get_number(jsn, id, 0);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 1);
      fwrite(&tmp, sizeof(f32), 1, stream);
    }

    inline void write_json_vec3(FILE *stream, const Json &jsn, u64 id)
    {
      f32 tmp;
      tmp = (f32)json::get_number(jsn, id, 0);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 1);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 2);
      fwrite(&tmp, sizeof(f32), 1, stream);
    }

    inline void write_json_vec4(FILE *stream, const Json &jsn, u64 id)
    {
      f32 tmp;
      tmp = (f32)json::get_number(jsn, id, 0);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 1);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 2);
      fwrite(&tmp, sizeof(f32), 1, stream);
      tmp = (f32)json::get_number(jsn, id, 3);
      fwrite(&tmp, sizeof(f32), 1, stream);
    }

    inline void write_json_mat4(FILE *stream, const Json &jsn, u64 id)
    {
      f32 tmp;
      for (i32 i = 0; i < 16; i++) {
        tmp = (f32)json::get_number(jsn, id, i);
        fwrite(&tmp, sizeof(f32), 1, stream);
      }
    }

    inline void read_json_pose(const Json &jsn, u64 id, PoseResource &p)
    {
      const f32 default_translation = 0.0f;
      const f32 default_scale = 1.0f;
      const f32 default_rotation = 0.0f;

      if (!json::has(jsn, id, "translation")) {
        p.tx = p.ty = p.tz = default_translation;
      }
      else {
        u64 translation = json::get_id(jsn, id, "translation");
        p.tx = (f32)json::get_number(jsn, translation, 0);
        p.ty = (f32)json::get_number(jsn, translation, 1);
        p.tz = (json::size(jsn, translation) == 3) ? (f32)json::get_number(jsn, translation, 2) : 0.f;
      }

      if (!json::has(jsn, id, "scale")) {
        p.sx = p.sy = default_scale;
      }
      else {
        u64 scale = json::get_id(jsn, id, "scale");
        p.sx = (f32)json::get_number(jsn, scale, 0);
        p.sy = (f32)json::get_number(jsn, scale, 1);
      }
      
      if (!json::has(jsn, id, "rotation")) {
        p.rx = p.ry = p.rz = default_rotation;
      }
      else {
        u64 rotation = json::get_id(jsn, id, "rotation");
        p.rx = glm::radians((f32)json::get_number(jsn, rotation, 0));
        p.ry = glm::radians((f32)json::get_number(jsn, rotation, 1));
        p.rz = glm::radians((f32)json::get_number(jsn, rotation, 2));
      }
    }

    inline void write_json_pose(FILE *stream, const Json &jsn, u64 id)
    {
      PoseResource p;
      read_json_pose(jsn, id, p);
      fwrite(&p, sizeof(PoseResource), 1, stream);
    }

    inline void read_json_color(const Json &jsn, u64 id, u8* c, const char *attribute)
    {
      if (!json::has(jsn, id, attribute)) {
        c[0] = c[1] = c[2] = c[3] = 255;
        return;
      }
      const u64 color = json::get_id(jsn, id, attribute);
      c[0] = (u8)json::get_integer(jsn, color, 0);
      c[1] = (u8)json::get_integer(jsn, color, 1);
      c[2] = (u8)json::get_integer(jsn, color, 2);
      c[3] = (u8)json::get_integer(jsn, color, 3);
    }

    inline void read_json_texture(Work &w, const Json &jsn, u64 id, u32 *tex_name, u32 *tex_region)
    {
      const char *str = json::get_string(jsn, id, "texture");
      const char *region = strpbrk(str, "#[");

      *tex_region = 0;
      if (region) {
        char texture_name[MAX_PATH];
        const u32 texture_len = region - str;
        XASSERT(texture_len < MAX_PATH, "Texture name too long.");

        if (*region == '#') { // atlas
          *tex_region = compiler::create_id_string(w, region + 1);
        } else {  // tileset
          u32 row, col;
          sscanf(region, "[%d,%d]", &row, &col);
          *tex_region  = row << 16;
          *tex_region |= col & 0xFFFF;
        }
        strncpy(texture_name, str, texture_len);
        texture_name[texture_len] = '\0';
        *tex_name = compiler::create_reference(w, RESOURCE_TYPE_TEXTURE, texture_name);
      } else {
        *tex_name = compiler::create_reference(w, RESOURCE_TYPE_TEXTURE, str);
      }
    }
  }

  inline CompilerBase::CompilerBase(Allocator &_a, ResourceType _type)
    : a(&_a), type(_type){}

  inline bool CompilerBase::compile_stream(Work &w)
  {
    (void)w;
    return true;
  }

  inline bool CompilerBase::has_stream()
  {
    return false;
  }

  inline JsonCompiler::JsonCompiler(Allocator &a, ResourceType _type, StringPool &sp)
    : CompilerBase(a, _type), jsn(a, sp) {}

  inline BinaryCompiler::BinaryCompiler(Allocator &a, ResourceType _type)
    : CompilerBase(a, _type), buf(a) {}

  inline bool BinaryCompiler::load_bytes(Work &w)
  {
    return compiler::load_bytes(w.src, buf);
  }

  inline bool JsonCompiler::load_json(Work &w)
  {
    return compiler::load_json(w, w.src, type, jsn);
  }
}