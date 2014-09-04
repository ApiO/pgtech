#pragma once

#include <stdio.h>
#include <data/sound.h>
#include <runtime/memory.h>

namespace pge
{
  namespace sound_resource
  {
    void  register_type(void);
    void *load(FILE *file, u32 name, u32 size);
    void  bring_in(void *data, u32 name);
    void  bring_out(void *data, u32 name);

    i32  is_vorbis(const SoundResource *sound);
    i32  loop(const SoundResource *sound);
    const f32 *data(const SoundResource *sound);
  }

  namespace sound_resource
  {
    inline void register_type(void)
    {
      resource_manager::register_type(RESOURCE_TYPE_SOUND,
                                      sound_resource::load,
                                      sound_resource::bring_in,
                                      resource_manager::default_patch_up,
                                      sound_resource::bring_out,
                                      resource_manager::default_unload);
    }
    
    inline i32 is_vorbis(const SoundResource *sound)
    {
      return sound->_flags & SoundResource::Flags::VORBIS;
    }

    inline i32 loop(const SoundResource *sound)
    {
      return sound->_flags & SoundResource::Flags::LOOP;
    }

    inline i32 single_instance(const SoundResource *sound)
    {
      return sound->_flags & SoundResource::Flags::SINGLE_INSTANCE;
    }

    inline const f32 *data(const SoundResource *sound)
    {
      return (f32*)memory::pointer_add(sound, sizeof(SoundResource));
    }
  }
}