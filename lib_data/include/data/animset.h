#pragma once

#include <runtime/types.h>

namespace pge
{
  enum EventType
  {
    EVENT_TYPE_BEAT = 0,
    EVENT_TYPE_TRIGGER
  };

  enum TrackType {
    TRACK_TYPE_BONE = 0,
    TRACK_TYPE_SPRITE,
    TRACK_TYPE_EVENT
  };

  enum ChannelType {
    CHANNEL_TYPE_TRANSLATION = 0,
    CHANNEL_TYPE_ROTATION,
    CHANNEL_TYPE_SCALE,
    CHANNEL_TYPE_FRAME,
    CHANNEL_TYPE_COLOR
  };

  enum CurveType {
    CURVE_TYPE_LINEAR  = 0,
    CURVE_TYPE_STEPPED = 1,
    CURVE_TYPE_BEZIER  = 2
  };

  struct BezierCurve {
    f32 cx1, cy1, cx2, cy2;
  };

  struct AnimsetResource
  {
    struct BoneTrack
    {
      u32 name;
      f32 bone_length;
    };

    struct SpriteTrack
    {
      u32 name;
    };

    struct EventTrack
    {
      u32 name;
      u32 type;
    };

    struct AnimationHeader {
      u32 name;
      u32 data_offset;
    };

    struct KeyframeHeader {
      f32 time;
      u16 track_index;
    };

    u16 num_bone_tracks;
    u16 num_sprite_tracks;
    u16 num_event_tracks;
    u16 num_animations;
  };
}
