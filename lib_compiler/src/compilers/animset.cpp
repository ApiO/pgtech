#include <algorithm>

#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/hash.h>
#include <runtime/temp_allocator.h>

#include <data/animset.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;
  using namespace pge::string_stream;
  using namespace pge::compiler;

  union TrackId
  {
    struct {
      TrackType type;
      u32 name;
    } fields;
    u64 as64;
  };

  union FrameId
  {
    struct {
      u32 track_index;
      u32 name;
    } fields;
    u64 as64;
  };

  struct KeyframeKey
  {
    f32 time;
    f32 time_needed;
    u16 data_index;
  };

  struct KeyframeData
  {
    u16 track_index;
    ChannelType track_channel;
    u64 value_id;
    u64 curve_id;
  };

  static bool keyframe_pred_local(const KeyframeKey &a, const KeyframeKey b)
  {
    return a.time < b.time;
  }

  static void append_keyframes(Array<KeyframeKey> &keys, Array<KeyframeData> &data,
                               Json &jsn, u64 timeline, TrackType track_type, u16 track_index, ChannelType track_channel)
  {
    // append bone keys
    const u32 first = array::size(keys);
    const u32 size  = json::size(jsn, timeline);

    KeyframeKey  fkey;
    KeyframeData fdata;
    fdata.track_index   = track_index;
    fdata.track_channel = track_channel;

    for (u32 i = 0; i < size; i++) {
      u64 node = json::get_id(jsn, timeline, i);

      if (track_type == TRACK_TYPE_EVENT) {
        fkey.time = (f32)json::get_node(jsn, node).value.number;
      }
      else {
        fkey.time = (f32)json::get_number(jsn, node, "time");
        fdata.value_id = json::get_node(jsn, node, "value").id;

        if (track_channel == CHANNEL_TYPE_COLOR || track_type == TRACK_TYPE_BONE)
          fdata.curve_id = json::has(jsn, node, "curve") ? json::get_id(jsn, node, "curve") : json::NO_NODE;
      }
      fkey.data_index = (u16)array::size(data);
      array::push_back(keys, fkey);
      array::push_back(data, fdata);
    }

    std::sort(&keys[first], &keys[first + size], keyframe_pred_local);

    f32 previous_time = 0.0f;
    for (u32 i = first; i < first + size; i++) {
      keys[i].time_needed = previous_time;
      previous_time = keys[i].time;
    }

    if (track_channel != CHANNEL_TYPE_ROTATION)
      return;

    f32 last_angle = 0.0f;
    for (u32 i = first; i < first + size; i++) {
      Json::Node &n = json::get_node(jsn, data[keys[i].data_index].value_id);
      f32 angle = (f32)(n.type == JSON_INTEGER ? n.value.integer : n.value.number);
      f32 amount = angle - last_angle;
      while (amount > 180)
        amount -= 360;
      while (amount < -180)
        amount += 360;

      amount = last_angle + amount;
      while (amount > 180)
        amount -= 360;
      while (amount < -180)
        amount += 360;
      n.type = JSON_NUMBER;
      n.value.number = amount;

      last_angle = angle;
    }
  }

  static bool keyframe_pred_global(const KeyframeKey &a, const KeyframeKey b)
  {
    return a.time_needed < b.time_needed ||
      (a.time_needed == b.time_needed && a.time < b.time);
  }

  // sort keyframe keys by time needed and return the duration (keyfrime with the max time)
  static inline f32 sort_keyframes(Array<KeyframeKey> &keyframes)
  {
    std::sort(array::begin(keyframes), array::end(keyframes), keyframe_pred_global);
    f32 duration = 0;
    for (u32 i = 0; i < array::size(keyframes); i++) {
      if (keyframes[i].time > duration)
        duration = keyframes[i].time;
    }
    return duration;
  }

  static bool get_curve_type(const Json &jsn, u64 curve_id, CurveType &type)
  {
    if (curve_id == json::NO_NODE)
    {
      type = CURVE_TYPE_LINEAR;
      return true;
    }

    // if there is a curve specified, write its type.
    const Json::Node &curve = json::get_node(jsn, curve_id);

    if (curve.type != JSON_ARRAY && curve.type != JSON_STRING){
      LOG("Wrong type for a curve definition.");
      return false;
    }

    if (curve.type == JSON_STRING) {
      if (strcmp(curve.value.string, "LINEAR") != 0
          && strcmp(curve.value.string, "STEPPED") != 0){
        LOG("Wrong value fur a curve attribute.");
        return false;
      }

      if (strcmp(curve.value.string, "LINEAR") == 0)
        type = CURVE_TYPE_LINEAR;
      else
        type = CURVE_TYPE_STEPPED;
    }
    else {
      type = CURVE_TYPE_BEZIER;
    }
    return true;
  }

  // writes the number of key for each channel of the animation
  void write_num_keys(const Json &jsn, u64 tracks, u64 animation, FILE* stream)
  {
    TempAllocator4096 ta;

    Array<u32> num_translation_keys(ta);
    Array<u32> num_rotation_keys(ta);
    Array<u32> num_scale_keys(ta);
    Array<u32> num_frame_keys(ta);
    Array<u32> num_color_keys(ta);
    Array<u32> num_event_keys(ta);

    if (json::has(jsn, tracks, "bone")) {
      const u64 set_tracks = json::get_id(jsn, tracks, "bone");
      const i32 num_tracks = json::size(jsn, set_tracks);
      array::resize(num_translation_keys, num_tracks);
      array::resize(num_rotation_keys, num_tracks);
      array::resize(num_scale_keys, num_tracks);

      for (i32 i = 0; i < num_tracks; i++) {
        num_translation_keys[i] = 0;
        num_rotation_keys[i] = 0;
        num_scale_keys[i] = 0;

        const Json::Node &set_track = json::get_node(jsn, set_tracks, i);
        if (json::has(jsn, animation, "bone")) {
          const u64 anim_tracks = json::get_id(jsn, animation, "bone");
          if (json::has(jsn, anim_tracks, set_track.name)) {
            const u64 anim_track = json::get_id(jsn, anim_tracks, set_track.name);
            if (json::has(jsn, anim_track, "translation"))
              num_translation_keys[i] = json::size(jsn, anim_track, "translation");
            if (json::has(jsn, anim_track, "rotation"))
              num_rotation_keys[i] = json::size(jsn, anim_track, "rotation");
            if (json::has(jsn, anim_track, "scale"))
              num_scale_keys[i] = json::size(jsn, anim_track, "scale");
          }
        }
      }
    }

    if (json::has(jsn, tracks, "sprite")) {
      const u64 set_tracks = json::get_id(jsn, tracks, "sprite");
      const i32 num_tracks = json::size(jsn, set_tracks);
      array::resize(num_frame_keys, num_tracks);
      array::resize(num_color_keys, num_tracks);
      for (i32 i = 0; i < num_tracks; i++) {
        num_frame_keys[i] = 0;
        num_color_keys[i] = 0;

        const Json::Node &set_track = json::get_node(jsn, set_tracks, i);
        if (json::has(jsn, animation, "sprite")) {
          const u64 anim_tracks = json::get_id(jsn, animation, "sprite");
          if (json::has(jsn, anim_tracks, set_track.name)) {
            const u64 anim_track = json::get_id(jsn, anim_tracks, set_track.name);
            if (json::has(jsn, anim_track, "frame"))
              num_frame_keys[i] = json::size(jsn, anim_track, "frame");
            if (json::has(jsn, anim_track, "color"))
              num_color_keys[i] = json::size(jsn, anim_track, "color");
          }
        }
      }
    }

    if (json::has(jsn, tracks, "event")) {
      const u64 set_tracks = json::get_id(jsn, tracks, "event");
      const i32 num_tracks = json::size(jsn, set_tracks);
      array::resize(num_event_keys, num_tracks);
      for (i32 i = 0; i < num_tracks; i++) {
        num_frame_keys[i] = 0;
        num_color_keys[i] = 0;

        const Json::Node &set_track = json::get_node(jsn, set_tracks, i);
        if (json::has(jsn, animation, "sprite")) {
          const u64 anim_tracks = json::get_id(jsn, animation, "sprite");
          if (json::has(jsn, anim_tracks, set_track.name)) {
            num_event_keys[i] = json::size(jsn, anim_tracks, set_track.name);
          }
        }
      }
    }

    // write all that fecking shit...
    fwrite(array::begin(num_translation_keys), sizeof(u32), array::size(num_translation_keys), stream);
    fwrite(array::begin(num_rotation_keys), sizeof(u32), array::size(num_rotation_keys), stream);
    fwrite(array::begin(num_scale_keys), sizeof(u32), array::size(num_scale_keys), stream);
    fwrite(array::begin(num_frame_keys), sizeof(u32), array::size(num_frame_keys), stream);
    fwrite(array::begin(num_color_keys), sizeof(u32), array::size(num_color_keys), stream);
    fwrite(array::begin(num_event_keys), sizeof(u32), array::size(num_event_keys), stream);
  }
}

namespace pge
{

  AnimsetCompiler::AnimsetCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_ANIMSET, sp) {}

  bool AnimsetCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;
    const u16 DEFAULT_INDEX = 0xFFFF;

    Hash<u16> track_id_to_index(*a);
    Hash<u16> frame_id_to_index(*a);

    // ---------------------------------------------------------------
    // Track declarations
    // ---------------------------------------------------------------
    const u64 tracks = json::get_id(jsn, json::root(jsn), "tracks");
    const u64 animations = json::get_id(jsn, json::root(jsn), "animations");

    AnimsetResource set;

    set.num_bone_tracks = (u16)json::size(jsn, tracks, "bone");
    set.num_sprite_tracks = (u16)json::size(jsn, tracks, "sprite");
    set.num_event_tracks = (u16)json::size(jsn, tracks, "event");
    set.num_animations = (u16)json::size(jsn, animations);
    fwrite(&set, sizeof(AnimsetResource), 1, w.data);

    {
      u16 track_index = 0;
      TrackId track_id;

      // write bone track declarations
      if (set.num_bone_tracks > 0) {
        track_id.fields.type = TRACK_TYPE_BONE;

        u64 next = json::get_node(jsn, tracks, "bone").child;
        while (next != json::NO_NODE) {
          // write the track name
          const Json::Node &bone = json::get_node(jsn, next);

          const u32 name = compiler::create_id_string(w, bone.name);
          fwrite(&name, sizeof(u32), 1, w.data);

          const f32 bone_length = (f32)json::get_number(jsn, bone.id, "length", 0.0f);
          fwrite(&bone_length, sizeof(f32), 1, w.data);

          // store the track index
          track_id.fields.name = name;
          hash::set(track_id_to_index, track_id.as64, track_index++);

          next = bone.next;
        }
      }

      // write sprite track declarations
      if (set.num_sprite_tracks > 0) {
        track_id.fields.type = TRACK_TYPE_SPRITE;

        u64 next = json::get_node(jsn, tracks, "sprite").child;
        while (next != json::NO_NODE) {
          const Json::Node &sprite = json::get_node(jsn, next);

          // write the track name 
          track_id.fields.name = compiler::create_id_string(w, sprite.name);
          fwrite(&track_id.fields.name, sizeof(u32), 1, w.data);

          // populate frame_id_to_index
          const u16 num_frames = (u16)json::size(jsn, sprite.id);
          if (num_frames > 0) {
            FrameId frame_id;
            frame_id.fields.track_index = track_index;

            for (u16 i = 0; i < num_frames; i++) {
              frame_id.fields.name = compiler::create_id_string(w, json::get_string(jsn, sprite.id, i));
              hash::set(frame_id_to_index, frame_id.as64, i);
            }
          }

          // next sprite track
          hash::set(track_id_to_index, track_id.as64, track_index++);
          next = sprite.next;
        }
      }

      // write event track declarations
      if (set.num_event_tracks > 0) {
        track_id.fields.type = TRACK_TYPE_EVENT;

        u64 next = json::get_node(jsn, tracks, "event").child;
        while (next != json::NO_NODE) {
          // write event name
          const Json::Node &event = json::get_node(jsn, next);
          const u32 name = compiler::create_id_string(w, event.name);
          fwrite(&name, sizeof(u32), 1, w.data);

          // write the event type
          u32 type = 0xFFFFFFu;
          if (strcmp(event.value.string, "BEAT") == 0)         type = EVENT_TYPE_BEAT;
          else if (strcmp(event.value.string, "TRIGGER") == 0) type = EVENT_TYPE_TRIGGER;
          fwrite(&type, sizeof(u32), 1, w.data);

          // store the track index
          track_id.fields.name = name;
          hash::set(track_id_to_index, track_id.as64, track_index++);

          next = event.next;
        }
      }
    }

    // ---------------------------------------------------------------
    // Animation declarations
    // ---------------------------------------------------------------
    {
      Array<KeyframeKey>  keyframe_keys(*a);
      Array<KeyframeData> keyframe_data(*a);
      Array<AnimsetResource::AnimationHeader> anim_headers(*a);
      Array<u32> num_keyframes(*a);
      u32 anim_headers_offset = ftell(w.data);
      array::resize(anim_headers, set.num_animations);

      // leave space to write anim_headers later
      fseek(w.data, sizeof(AnimsetResource::AnimationHeader) * set.num_animations, SEEK_CUR);

      for (u16 i = 0; i < set.num_animations; i++) {
        const Json::Node &anim = json::get_node(jsn, animations, i);
        anim_headers[i].name = compiler::create_id_string(w, anim.name);
        anim_headers[i].data_offset = ftell(w.data);

        array::clear(keyframe_keys);
        array::clear(keyframe_data);

        TrackId track_id;

        if (json::has(jsn, anim.id, "bone")) {
          track_id.fields.type = TRACK_TYPE_BONE;

          u64 next_bone = json::get_node(jsn, anim.id, "bone").child;
          while (next_bone != json::NO_NODE) {
            const Json::Node &bone_track = json::get_node(jsn, next_bone);

            track_id.fields.name = compiler::create_id_string(w, bone_track.name);
            u16 track_index = hash::get(track_id_to_index, track_id.as64, DEFAULT_INDEX);

            if (json::has(jsn, next_bone, "translation"))
              append_keyframes(keyframe_keys, keyframe_data, jsn, json::get_id(jsn, next_bone, "translation"), track_id.fields.type, track_index, CHANNEL_TYPE_TRANSLATION);

            if (json::has(jsn, next_bone, "rotation"))
              append_keyframes(keyframe_keys, keyframe_data, jsn, json::get_id(jsn, next_bone, "rotation"), track_id.fields.type, track_index, CHANNEL_TYPE_ROTATION);

            if (json::has(jsn, next_bone, "scale"))
              append_keyframes(keyframe_keys, keyframe_data, jsn, json::get_id(jsn, next_bone, "scale"), track_id.fields.type, track_index, CHANNEL_TYPE_SCALE);

            next_bone = bone_track.next;
          }
        }

        if (json::has(jsn, anim.id, "sprite")) {
          track_id.fields.type = TRACK_TYPE_SPRITE;

          u64 next_sprite = json::get_node(jsn, anim.id, "sprite").child;
          while (next_sprite != json::NO_NODE) {
            const Json::Node &sprite_track = json::get_node(jsn, next_sprite);
            track_id.fields.name = compiler::create_id_string(w, sprite_track.name);

            const u16 track_index = hash::get(track_id_to_index, track_id.as64, DEFAULT_INDEX);

            if (json::has(jsn, next_sprite, "frame"))
              append_keyframes(keyframe_keys, keyframe_data, jsn, json::get_id(jsn, next_sprite, "frame"), track_id.fields.type, track_index, CHANNEL_TYPE_FRAME);

            if (json::has(jsn, next_sprite, "color")) {
              append_keyframes(keyframe_keys, keyframe_data, jsn, json::get_id(jsn, next_sprite, "color"), track_id.fields.type, track_index, CHANNEL_TYPE_COLOR);
            }
            next_sprite = sprite_track.next;
          }
        }

        if (json::has(jsn, anim.id, "event")) {
          track_id.fields.type = TRACK_TYPE_EVENT;
          u64 next_event = json::get_node(jsn, anim.id, "event").child;

          while (next_event != json::NO_NODE) {
            const Json::Node &event_track = json::get_node(jsn, next_event);
            track_id.fields.name = compiler::create_id_string(w, event_track.name);
            u16 track_index = hash::get(track_id_to_index, track_id.as64, DEFAULT_INDEX);
            append_keyframes(keyframe_keys, keyframe_data, jsn, next_event, track_id.fields.type, track_index, CHANNEL_TYPE_COLOR);
            next_event = event_track.next;
          }
        }

        // ----------------------------------------------

        {
          f32 duration = sort_keyframes(keyframe_keys);
          fwrite(&duration, sizeof(f32), 1, w.data);
          write_num_keys(jsn, tracks, anim.id, w.data);

          // write the keyframes
          for (u32 i = 0; i < array::size(keyframe_keys); ++i) {
            const KeyframeData &kf_data = keyframe_data[keyframe_keys[i].data_index];

            fwrite(&keyframe_keys[i].time, sizeof(f32), 1, w.data);
            fwrite(&kf_data.track_index, sizeof(u16), 1, w.data);

            if (kf_data.track_index < set.num_bone_tracks) {
              // bone track
              CurveType curve_type;
              if (!get_curve_type(jsn, kf_data.curve_id, curve_type)) 
                return false;

              const u8 channel = (u8)kf_data.track_channel | (u8)curve_type << 4;
              f32 tmp = 0;
              const Json::Node *tmp_node;

              fwrite(&channel, sizeof(u8), 1, w.data);
              switch (kf_data.track_channel) {
                case CHANNEL_TYPE_TRANSLATION:
                  write_json_vec2(w.data, jsn, kf_data.value_id);
                  break;
                case CHANNEL_TYPE_ROTATION:
                  tmp_node = &json::get_node(jsn, kf_data.value_id);
                  tmp = (f32)(tmp_node->type == JSON_INTEGER ? tmp_node->value.integer : tmp_node->value.number);
                  fwrite(&tmp, sizeof(f32), 1, w.data);
                  break;
                case CHANNEL_TYPE_SCALE:
                  write_json_vec2(w.data, jsn, kf_data.value_id);
                  break;
                default:
                  LOG("Unhandled ChannelType enum value %d", kf_data.track_channel);
                  return false;
              }
              if (curve_type == CURVE_TYPE_BEZIER)
                write_json_vec4(w.data, jsn, kf_data.curve_id);

            }
            else if (kf_data.track_index < set.num_bone_tracks + set.num_sprite_tracks) {
              // sprite track
              u16 channel;
              if (kf_data.track_channel == CHANNEL_TYPE_FRAME) {
                // frame
                FrameId fid;
                fid.fields.track_index = kf_data.track_index;
                fid.fields.name = compiler::create_id_string(w, json::get_node(jsn, kf_data.value_id).value.string);
                const u16 frame_index = hash::get(frame_id_to_index, fid.as64, DEFAULT_INDEX);
                if (frame_index == DEFAULT_INDEX){
                  LOG("frame not declared");
                  return false;
                }

                channel = (u16)(kf_data.track_channel | (frame_index << 4));
                fwrite(&channel, sizeof(u16), 1, w.data);
              }
              else {
                // color
                CurveType curve_type;
                if (!get_curve_type(jsn, kf_data.curve_id, curve_type)) 
                  return false;

                channel = (u16)(kf_data.track_channel | ((u16)curve_type << 4));
                fwrite(&channel, sizeof(u16), 1, w.data);

                u8 component;
                component = (u8)json::get_integer(jsn, kf_data.value_id, 0);
                fwrite(&component, sizeof(u8), 1, w.data);
                component = (u8)json::get_integer(jsn, kf_data.value_id, 1);
                fwrite(&component, sizeof(u8), 1, w.data);
                component = (u8)json::get_integer(jsn, kf_data.value_id, 2);
                fwrite(&component, sizeof(u8), 1, w.data);
                component = (u8)json::get_integer(jsn, kf_data.value_id, 3);
                fwrite(&component, sizeof(u8), 1, w.data);

                if (curve_type == CURVE_TYPE_BEZIER)
                  write_json_vec4(w.data, jsn, kf_data.curve_id);
              }
            }
            else {
              // event track
            }
          }
        }
      }

      // rewind and write animation headers
      fseek(w.data, anim_headers_offset, SEEK_SET);
      fwrite(array::begin(anim_headers), sizeof(AnimsetResource::AnimationHeader), array::size(anim_headers), w.data);
    }

    return true;
  }
}