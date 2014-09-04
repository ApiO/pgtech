#include <runtime/array.h>
#include <runtime/hash.h>
#include <runtime/pool_allocator.h>
#include <soloud.h>

#include "resource/resource_manager.h"
#include "audio_system.h"
#include "audio_resource.h"
#include "sound_resource.h"

// ---------------------------------------------------------------
// Internals declaration
// ---------------------------------------------------------------

namespace
{
  using namespace pge;
  using namespace SoLoud;

  class SoundSource : public AudioSource
  {
  public:
    SoundResource *resource;
    FILE *stream;
    u32   stream_start;
    u32   stream_refs;
    u32   name;
    AudioSourceInstance *createInstance();
    SoundSource(SoundResource *resource, u32 sr_name);
    virtual ~SoundSource() {}
  };

  class SoundInstance : public AudioSourceInstance
  {
    SoundSource *mParent;
    u32 mOffset;
  public:
    SoundInstance(SoundSource *aParent);
    virtual ~SoundInstance();
    virtual void getAudio(float *aBuffer, unsigned int aSamples);
    virtual result rewind();
    virtual bool   hasEnded();
  };

  struct AudioGlobals {
    const AudioResource *configuration;
    Soloud *soloud;
    Bus    *buses;
    i32    *bus_handles;
    Hash<SoundSource*> *sources;
    PoolAllocator *pool;

    AudioGlobals() : configuration(0), soloud(0), buses(0), bus_handles(0), sources(0) {}
  };

  AudioGlobals _audio_globals;

  inline int read16(FILE * f)
  {
    i16 i;
    fread(&i, sizeof(u16), 1, f);
    return i;
  }

  inline int read8(FILE * f)
  {
    i8 i;
    fread(&i, sizeof(u8), 1, f);
    return i;
  }

  void read_wav_stream(f32 *data, u32 data_offset, u32 data_samples, u32 channels, u32 bit_depth, u32 read_channels, u32 num_samples, FILE *file)
  {
    u32 i, j;

    if (bit_depth == 8) {
      for (i = 0; i < num_samples; i++) {
        for (j = 0; j < channels; j++) {
          if (j == 0) {
            data[data_offset + i] = read8(file) / (f32)0x80;
          } else {
            if (read_channels > 1 && j == 1)
              data[data_offset + i + data_samples] = read8(file) / (f32)0x80;
            else
              read8(file);
          }
        }
      }
    } else if (bit_depth == 16) {
      for (i = 0; i < num_samples; i++) {
        for (j = 0; j < channels; j++) {
          if (j == 0) {
            data[data_offset + i] = read16(file) / (f32)0x8000;
          } else {
            if (read_channels > 1 && j == 1)
              data[data_offset + i + data_samples] = read16(file) / (f32)0x8000;
            else
              read16(file);
          }
        }
      }
    }
  }
}

// ---------------------------------------------------------------
// Audio System implementation
// ---------------------------------------------------------------

namespace pge
{
  namespace audio_system
  {
    void init(u32 config_name, Allocator &a)
    {
      const AudioResource *configuration = (AudioResource*)resource_manager::get(RESOURCE_TYPE_AUDIO, config_name);

      void *p = a.allocate(sizeof(Soloud)+(sizeof(Bus)+sizeof(i32))*configuration->_num_buses + sizeof(Hash<SoundSource*>) + sizeof(PoolAllocator));
      _audio_globals.soloud = new(p)Soloud();
      p = memory::pointer_add(p, sizeof(Soloud));
      _audio_globals.buses = (Bus*)p;
      p = memory::pointer_add(p, sizeof(Bus)*configuration->_num_buses);
      _audio_globals.bus_handles = (i32*)p;
      p = memory::pointer_add(p, sizeof(i32)*configuration->_num_buses);
      _audio_globals.sources = new(p)Hash<SoundSource*>(a);
      p = memory::pointer_add(p, sizeof(Hash<SoundSource*>));
      _audio_globals.pool = new(p)PoolAllocator(sizeof(SoundSource), VOICE_COUNT, a);
      _audio_globals.configuration = configuration;

      // WMME backend... meh...
      _audio_globals.soloud->init();

      // init buses
      const AudioResource::Bus *cbuses = audio_resource::buses(configuration);
      for (i32 i = 0; i < configuration->_num_buses; i++) {
        new (_audio_globals.buses + i) Bus();
        _audio_globals.bus_handles[i] = cbuses[i].parent < 0 ?
          _audio_globals.soloud->play(_audio_globals.buses[i], cbuses[i].volume) :
          _audio_globals.buses[cbuses[i].parent].play(_audio_globals.buses[i], cbuses[i].volume);
      }
    }

    void set_bus_volume(const char *name, f32 volume)
    {
      const u32 h = murmur_hash_32(name);
      const i32 i = audio_resource::bus_index(_audio_globals.configuration, h);
      _audio_globals.soloud->setVolume(_audio_globals.bus_handles[i], volume);
    }

    void set_bus_pan(const char *name, f32 pan)
    {
      const u32 h = murmur_hash_32(name);
      const i32 i = audio_resource::bus_index(_audio_globals.configuration, h);
      _audio_globals.soloud->setPan(_audio_globals.bus_handles[i], pan);
    }

    u64 trigger_sound(AudioWorld &world, const char *name, const glm::vec3 &position);

    u64 trigger_sound(AudioWorld &world, const char *name)
    {
      (void)world;
      const u32 h = murmur_hash_32(name);
      SoundSource *ss = hash::get(*_audio_globals.sources, h, (SoundSource*)NULL);
      XASSERT(ss, "could not find the sound named %u.", name);
      const u32 i = audio_resource::bus_index(_audio_globals.configuration, ss->resource->_bus);

      return _audio_globals.buses[i].play(*ss);
    }

    void set_position(AudioWorld &world, u64 playing_sound, const glm::vec3 &position);

    void stop(AudioWorld &world, u64 playing_sound)
    {
      (void)world;
      _audio_globals.soloud->stop((int)playing_sound);
    }

    void stop_all(AudioWorld &world)
    {
      (void)world;
      Hash<SoundSource*>::Entry *e, *end = hash::end(*_audio_globals.sources);
      for (e = hash::begin(*_audio_globals.sources); e < end; e++)
        _audio_globals.soloud->stopAudioSource(*e->value);
    }

    void update(AudioWorld &world);

    void shutdown()
    {
      _audio_globals.soloud->deinit();
      _audio_globals.soloud->~Soloud();
      _audio_globals.pool->~PoolAllocator();
      _audio_globals.sources->~Hash();
      _audio_globals.sources->_data._allocator->deallocate(_audio_globals.soloud);
    }
  }
}

// ---------------------------------------------------------------
// Sound Resource callbacks
// ---------------------------------------------------------------

namespace pge
{
  namespace sound_resource
  {
    void *load(FILE *file, u32 name, u32 size)
    {
      (void)name, size;
      SoundResource header;
      fread(&header, sizeof(SoundResource), 1, file);
      const u32 read_channels = header.num_channels > 1 ? 2 : 1;
      void *resource = memory_globals::default_allocator().allocate(
        sizeof(SoundResource)+header.num_samples * read_channels * sizeof(f32));

      memcpy(resource, &header, sizeof(SoundResource));
      f32 *data = (f32*)memory::pointer_add(resource, sizeof(SoundResource));
      read_wav_stream(data, 0, header.num_samples, header.num_channels, header.bit_depth, read_channels, header.num_samples, file);

      return resource;
    }

    // creates an audio source
    void bring_in(void *data, u32 name)
    {
      SoundResource *sd = (SoundResource*)data;
      SoundSource *ss = MAKE_NEW(*_audio_globals.pool, SoundSource, sd, name);
      hash::set(*_audio_globals.sources, name, ss);
    }

    // destroys the audio source
    void bring_out(void *data, u32 name)
    {
      (void)data;
      SoundSource *ss = hash::get(*_audio_globals.sources, name, (SoundSource*)NULL);
      _audio_globals.soloud->stopAudioSource(*ss);
      MAKE_DELETE(*_audio_globals.pool, SoundSource, ss);
      hash::remove(*_audio_globals.sources, name);
    }
  }
}

// ---------------------------------------------------------------
// Soloud implementations
// ---------------------------------------------------------------

namespace
{
  using namespace pge;
  using namespace SoLoud;

  SoundSource::SoundSource(SoundResource *sr, u32 sr_name) : resource(sr), stream(0), stream_refs(0), name(sr_name)
  {
    mBaseSamplerate = (f32)sr->sampling_rate;
    mChannels = sr->num_channels;
    mFlags = 0;
    if (sound_resource::loop(sr))
      mFlags |= AudioSource::FLAGS::SHOULD_LOOP;

    if (sound_resource::single_instance(sr))
      mFlags |= AudioSource::FLAGS::SINGLE_INSTANCE;
  }

  AudioSourceInstance *SoundSource::createInstance()
  {
    //return MAKE_NEW(*_audio_globals.pool, SoundInstance, this);
    return new SoundInstance(this);
  }

  SoundInstance::SoundInstance(SoundSource *aParent)
  {
    mParent = aParent;
    mOffset = 0;
    ++aParent->stream_refs;

    if (aParent->stream_refs == 1) {
      u8 c;
      aParent->stream = resource_manager::open_stream(RESOURCE_TYPE_SOUND, aParent->name);
      aParent->stream_start = ftell(aParent->stream);

      // do a read & rewind to be sure the system starts caching the stream
      fread(&c, sizeof(u8), 1, aParent->stream);
      fseek(aParent->stream, aParent->stream_start, 0);
    }
  }

  SoundInstance::~SoundInstance()
  {
    --mParent->stream_refs;
    if (!mParent->stream_refs)
      fclose(mParent->stream);

    //MAKE_DELETE(*_audio_globals.pool, SoundInstance, this);
  }

  void SoundInstance::getAudio(float *aBuffer, unsigned int aSamples)
  {
    if (!mParent->resource)
      return;

    u32 memory_samples = mParent->resource->num_samples;
    u32 stream_samples = mParent->resource->num_stream_samples;
    u32 total_samples = memory_samples + stream_samples;

    // Buffer size may be bigger than samples, and samples may loop..
    u32 written = 0;
    u32 maxwrite = (aSamples > total_samples) ? total_samples : aSamples;

    while (written < aSamples) {
      u32 copy_size = maxwrite;
      if (copy_size + mOffset > total_samples)
        copy_size = total_samples - mOffset;

      if (copy_size + written > (u32)aSamples)
        copy_size = aSamples - written;

      u32 memory_size = mOffset > memory_samples ? 0 : copy_size + mOffset > memory_samples ? memory_samples - mOffset : copy_size;
      u32 stream_size = copy_size - memory_size;

      if (memory_size) {
        for (u32 i = 0; i < mChannels; i++) {
          memcpy(aBuffer + i * aSamples + written,
                 sound_resource::data(mParent->resource) + mOffset + i * memory_samples,
                 sizeof(float)* memory_size);
        }
      }

      if (stream_size) {
        u32 stream_pos = ftell(mParent->stream);
        u32 stream_offset = mOffset > memory_samples ? 
          (mOffset - (memory_samples - memory_size)) * mParent->resource->num_channels * mParent->resource->bit_depth/8 + mParent->stream_start : mParent->stream_start;

        if (stream_pos != stream_offset)
          fseek(mParent->stream, stream_offset, 0);

        read_wav_stream(aBuffer,
                        memory_size + written,
                        copy_size,
                        mParent->resource->num_channels,
                        mParent->resource->bit_depth,
                        mChannels,
                        stream_size,
                        mParent->stream);
      }

      written += copy_size;
      mOffset += copy_size;

      if (copy_size != maxwrite) {
        if (mFlags & AudioSourceInstance::LOOPING) {
          if (mOffset == total_samples)
            mOffset = 0;
        } else {
          for (u32 i = 0; i < mChannels; i++)
            memset(aBuffer + copy_size + i * aSamples, 0, sizeof(float)* (aSamples - written));
          mOffset += aSamples - written;
          written = aSamples;
        }
      }
    }
  }

  result SoundInstance::rewind()
  {
    mOffset = 0;
    mStreamTime = 0;
    return 0;
  }

  bool SoundInstance::hasEnded()
  {
    if (!(mFlags & AudioSourceInstance::LOOPING) 
        && mOffset >= mParent->resource->num_samples + mParent->resource->num_stream_samples)
      return true;
    return false;
  }
}