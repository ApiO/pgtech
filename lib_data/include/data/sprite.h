#pragma once

#include <runtime/types.h>
#include "types.h"

namespace pge
{
  enum SequenceMode
  {
    SEQUENCE_MODE_FORWARD = 0,
    SEQUENCE_MODE_BACKWARD,
    SEQUENCE_MODE_FORWARD_LOOP,
    SEQUENCE_MODE_BACKWARD_LOOP,
    SEQUENCE_MODE_PINGPONG,
    SEQUENCE_MODE_RANDOM
  };

  struct SpriteResource
  {
    struct Frame {
      u32 name;
      u32 texture_name;
      u32 texture_region;
      PoseResource texture_pose;
      u8  color[4];
    };

    u16 num_frames;
    u16 setup_frame;
    u16 sequence_fps;
    u16 sequence_mode;
    u32 sequence_count;
    u32 _sequence_offset;
    u8  blend_mode;
  }; // folowed by the frames & sequence frames
}
