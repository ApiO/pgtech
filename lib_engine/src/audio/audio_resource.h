#pragma once
#include <data/audio.h>
#include <runtime/memory.h>
#include <runtime/assert.h>
#include <runtime/murmur_hash.h>

namespace pge
{
  namespace audio_resource
  {
    i32 num_buses(const AudioResource* audio);
    i32 bus_index(const AudioResource* audio, u32 name);
    const AudioResource::Bus *buses(const AudioResource* audio);
  }

  namespace audio_resource
  {
    inline i32 num_buses(const AudioResource* audio)
    {
      return audio->_num_buses;
    }

    inline const AudioResource::Bus *buses(const AudioResource* audio)
    {
      return (AudioResource::Bus*)memory::pointer_add(audio, sizeof(AudioResource)+audio->_num_buses * sizeof(u32));
    }

    inline i32 bus_index(const AudioResource* audio, u32 name)
    {
      const u32 *bus_names = (u32*)memory::pointer_add(audio, sizeof(AudioResource));
      i32 i = 0;

      while (i < audio->_num_buses) {
        if (bus_names[i] == name)
          return i;
        ++i;
      }

      XERROR("Could not find the bus named %s", name);
      return -1;
    }
  }
}