#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/temp_allocator.h>

#include <data/sprite.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;
  using namespace pge::compiler;

#define SEQUENCE 0x001
#define FPS      0x002

  static inline SequenceMode get_sequence_mode(const Json &jsn, const u64 &node)
  {
    const char *type = json::get_string(jsn, node, "mode", "novalue");

    if (strcmp(type, "novalue") == 0)       return SEQUENCE_MODE_FORWARD;
    if (strcmp(type, "BACKWARD") == 0)      return SEQUENCE_MODE_BACKWARD;
    if (strcmp(type, "FORWARD_LOOP") == 0)  return SEQUENCE_MODE_FORWARD_LOOP;
    if (strcmp(type, "BACKWARD_LOOP") == 0) return SEQUENCE_MODE_BACKWARD_LOOP;
    if (strcmp(type, "PINGPONG") == 0)      return SEQUENCE_MODE_PINGPONG;
    if (strcmp(type, "RANDOM") == 0)        return SEQUENCE_MODE_RANDOM;

    return SEQUENCE_MODE_FORWARD;
  }
}

namespace pge
{
  SpriteCompiler::SpriteCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_SPRITE, sp) {}

  bool SpriteCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    TempAllocator1024 ta(*a);
    Hash<u16> frame_name_to_index(ta);

    const Json &compilers_config = *w.project->compilers_config;
    const u64   def_conf_id    = json::get_node(compilers_config, json::root(compilers_config), "sprite").id;
    const u64   root = json::root(jsn);
    SpriteResource sprite;
    memset(&sprite, 0, sizeof(SpriteResource));

    // sets the blend mode
    const char *blend_mode = json::get_string(jsn, root, "blend_mode", "normal");
    if (strcmp(blend_mode, "normal") == 0)
      sprite.blend_mode = (u8)BLEND_MODE_NORMAL;
    else if (strcmp(blend_mode, "multiply") == 0)
      sprite.blend_mode = (u8)BLEND_MODE_MULTIPLY;
    else if (strcmp(blend_mode, "screen") == 0)
      sprite.blend_mode = (u8)BLEND_MODE_SCREEN;
    else if (strcmp(blend_mode, "overlay") == 0)
      sprite.blend_mode = (u8)BLEND_MODE_OVERLAY;
    else if (strcmp(blend_mode, "additive") == 0)
      sprite.blend_mode = (u8)BLEND_MODE_ADDITIVE;

    // set the number of frames
    const u64 frames = json::get_id(jsn, root, "frames");
    sprite.num_frames = (u16)json::size(jsn, frames);

    // populate frame_name_to_index
    for (u16 i = 0; i < sprite.num_frames; i++) {
      const u32 name = compiler::create_id_string(w, json::get_node(jsn, frames, i).name);
      hash::set(frame_name_to_index, (u64)name, i);
    }

    { // set the index of the setup frame
      const u64 setup_frame_id = (u64)compiler::create_id_string(w, json::get_string(jsn, root, "setup_frame", NULL));
      if (!hash::has(frame_name_to_index, setup_frame_id)) {
        LOG("Setup frame \"%s\" is not defined.", w.src);
        return false;
      }
      sprite.setup_frame = hash::get(frame_name_to_index, setup_frame_id, sprite.setup_frame);
    }

    // set the sequence options
    if (json::has(jsn, root, "sequence")) {
      sprite.sequence_fps     = (u16)json::get_integer(jsn, root, "fps", 0);
      sprite.sequence_mode    = (u16)get_sequence_mode(jsn, root);
      sprite.sequence_count   = json::size(jsn, root, "sequence");
      sprite._sequence_offset = sizeof(SpriteResource)+(sizeof(SpriteResource::Frame) * sprite.num_frames);
    }

    // write the header
    fwrite(&sprite, sizeof(SpriteResource), 1, w.data);

    // write the frames
    SpriteResource::Frame frame;
    for (u16 i = 0; i < sprite.num_frames; i++) {
      const Json::Node &n = json::get_node(jsn, frames, i);
      frame.name = compiler::create_id_string(w, n.name);

      // load the texture name, region & pose
      read_json_texture(w, jsn, n.id, &frame.texture_name, &frame.texture_region);
      compiler::read_json_pose(jsn, n.id, frame.texture_pose);     

      { // set color
        const Json *doc   = json::has(jsn, n.id, "color") ? &jsn : &compilers_config;
        const u64   color = json::has(jsn, n.id, "color") ? json::get_id(jsn, n.id, "color") : json::get_id(compilers_config, def_conf_id, "color");

        frame.color[0] = (u8)json::get_integer(*doc, color, 0);
        frame.color[1] = (u8)json::get_integer(*doc, color, 1);
        frame.color[2] = (u8)json::get_integer(*doc, color, 2);
        frame.color[3] = (u8)json::get_integer(*doc, color, 3);
      }

      // write the frame
      fwrite(&frame, sizeof(SpriteResource::Frame), 1, w.data);
    }

    // write the sequence frame indices
    for (u32 i = 0; i < sprite.sequence_count; i++) {
      const u64 sequence = json::get_id(jsn, root, "sequence");
      const u64 frame_name = (u64)compiler::create_id_string(w, json::get_string(jsn, sequence, i));
      if (!hash::has(frame_name_to_index, frame_name)) {
        LOG("Sequence frame \"%s\" is not defined.", w.src);
        return false;
      }
      const u16 frame_index = hash::get(frame_name_to_index, frame_name, frame_index);
      fwrite(&frame_index, sizeof(u16), 1, w.data);
    }
    return true;
  }
}