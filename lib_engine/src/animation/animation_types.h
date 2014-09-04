#pragma once

#include <glm/glm.hpp>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>

#include "animation/animset_resource.h"

namespace pge
{
  struct AnimationPlayer
  {
    AnimationPlayer(Allocator &a);

    struct SpriteTrack {
      u32 frame;
      u32 next_frame;
      f32 next_frame_time;
      BezierCurve next_color_curve;
      u32 num_frame_fetches;
      u32 num_color_fetches;
    };

    struct BoneTrack {
      BezierCurve next_translation_curve;
      BezierCurve next_rotation_curve;
      BezierCurve next_scale_curve;
      u32 num_translation_fetches;
      u32 num_rotation_fetches;
      u32 num_scale_fetches;
    };

    struct EventTrack {
      f32 time;
      u32 num_fetches;
    };

    template<typename T> struct CurvePoint {
      T   start_value;
      T   end_value;
      T   value;
      f32 start_time;
      f32 end_time;
      f32 dfx, dfy, ddfx, ddfy, dddfx, dddfy;
    };

    struct Playhead {
      void *start;
      void *position;
    };

    struct Session {
      u32  buf_size;
      f32  from, to;
      bool loop;
      f32  speed;
      f64  time;
      bool played;
      const AnimsetResource *set;
      const AnimsetResource::AnimationHeader
        *animation_header;
      const void  *animation_stream;
      const void  *playhead;
      BoneTrack   *bone_tracks;
      SpriteTrack *sprite_tracks;
      EventTrack  *event_tracks;
      CurvePoint<glm::vec2> *active_translation_points;
      CurvePoint<glm::vec2> *active_scale_points;
      CurvePoint<f32>  *active_rotation_points;
      CurvePoint<glm::vec4> *active_color_points;
      Array<u16>       *events;
    };

    IdLookupTable<Session> _sessions;
  };
}