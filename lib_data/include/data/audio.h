#pragma once

#include <runtime/types.h>

namespace pge
{
  struct AudioResource {
    enum Flags {
      ROUNDOFF_CLIPPING = 1,
      ENABLE_VISUALIZATION = 2
    };

    struct Bus {
      i32 parent; // -1 : no parent
      f32 volume;
      f32 pan;
    };

    u32 _flags;
    i32 _sampling_rate;
    i32 _buffer_size;
    i32 _num_buses;
  };

  // followed by :
  // bus_names
  // buses
}