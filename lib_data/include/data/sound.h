#pragma once

#include <runtime/types.h>

namespace pge
{
  struct SoundResource {
    enum Flags {
      LOOP = 1,
      VORBIS = 2,
      SINGLE_INSTANCE = 4,
    };

    u32 _flags;
    u32 _bus;
    f32 _range; // will be replaced by an attenuation curve

    u32 sampling_rate;
    u32 bit_depth;
    u32 num_channels;
    u32 num_samples;
    u32 num_stream_samples;
  };
}