#include "compiler_types.h"
#include "compiler.h"

#include <data/particle.h>

namespace pge
{
  ParticleCompiler::ParticleCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_PARTICLE, sp) {}

  bool ParticleCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    const u64 root = json::root(jsn);
    ParticleResource p;

    p.loop          =      json::get_bool    (jsn, root, "loop", false);
    p.lifespan      = (f32)json::get_number  (jsn, root, "lifespan");
    p.speed         = (f32)json::get_number  (jsn, root, "speed");
    p.rate          = (f32)json::get_number  (jsn, root, "rate");
    p.angle         = glm::radians((f32)json::get_number(jsn, root, "angle", 0));

    if (json::has(jsn, root, "box")) {
      const u64 box = json::get_id(jsn, root, "box");
      p.box[0] = (f32)json::get_number(jsn, box, 0);
      p.box[1] = (f32)json::get_number(jsn, box, 1);
      p.box[2] = (f32)json::get_number(jsn, box, 2);
    } else {
      memset(p.box, 0, 3*sizeof(f32));
    }

    if (json::has(jsn, root, "gravity")) {
      const u64 gravity = json::get_id(jsn, root, "gravity");
      p.gravity[0] = (f32)json::get_number(jsn, gravity, 0);
      p.gravity[1] = (f32)json::get_number(jsn, gravity, 1);
      p.gravity[2] = (f32)json::get_number(jsn, gravity, 2);
    } else {
      memset(p.gravity, 0, 3*sizeof(f32));
    }

    compiler::read_json_color(jsn, root, p.color_start, "color_start");
    if (json::has(jsn, root, "color_end"))
      compiler::read_json_color(jsn, root, p.color_end, "color_end");
    else
      memcpy(p.color_end, p.color_start, 4);

    if (json::has(jsn, root, "scale_start")) {
      const u64 scale = json::get_id(jsn, root, "scale_start");
      p.scale_start[0] = (f32)json::get_number(jsn, scale, 0);
      p.scale_start[1] = (f32)json::get_number(jsn, scale, 1);
      p.scale_start[2] = (f32)json::get_number(jsn, scale, 2);
    } else {
      p.scale_start[0] = p.scale_start[1] = p.scale_start[2] = 1;
    }

    if (json::has(jsn, root, "scale_end")) {
      const u64 scale = json::get_id(jsn, root, "scale_end");
      p.scale_end[0] = (f32)json::get_number(jsn, scale, 0);
      p.scale_end[1] = (f32)json::get_number(jsn, scale, 1);
      p.scale_end[2] = (f32)json::get_number(jsn, scale, 2);
    } else {
      memcpy(p.scale_end, p.scale_start, 3*sizeof(f32));
    }

    compiler::read_json_texture(w, jsn, root, &p.texture_name, &p.texture_region);

    fwrite(&p, sizeof(ParticleResource), 1, w.data);
    return true;
  }
}