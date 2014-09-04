#include <runtime/string_stream.h>
#include <runtime/trace.h>
#include <data/sound.h>

#include "compiler.h"

#define MAKEDWORD(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

namespace pge
{
  SoundCompiler::SoundCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_SOUND, sp), _buf(a) {}

  bool SoundCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    const char *name = json::get_string(jsn, json::root(jsn), "file");
    const Json &compilers_config = *w.project->compilers_config;
    const u64 def_conf_id = json::get_id(compilers_config, json::root(compilers_config), "sound");

    const f32  in_memory_duration = (f32)json::get_number(compilers_config, def_conf_id, "in_memory_duration") / 1000;
    const bool is_vorbis = strcmp(
      json::get_string(jsn, json::root(jsn), "compression",
      json::get_string(compilers_config, def_conf_id, "compression"))
      , "VORBIS") == 0;

    array::clear(_buf);
    if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA, name, _buf)) {
      LOG("Could not load the image \"%s\" in \"%s\"", name, w.src);
      return false;
    }

    void *p = array::begin(_buf);
    if (*(u32*)p != MAKEDWORD('R', 'I', 'F', 'F'))
      return false;
    p = memory::pointer_add(p, sizeof(u32));
    p = memory::pointer_add(p, sizeof(u32)); // wav size
    if (*(u32*)p != MAKEDWORD('W', 'A', 'V', 'E'))
      return false;
    p = memory::pointer_add(p, sizeof(u32));
    if (*(u32*)p != MAKEDWORD('f', 'm', 't', ' '))
      return false;

    p = memory::pointer_add(p, sizeof(u32));
    u32 subchunk1size = *(u32*)p;
    p = memory::pointer_add(p, sizeof(u32));
    void *block_start = p;

    u32 audioformat = *(u16*)p;
    p = memory::pointer_add(p, sizeof(u16));
    u32 channels = *(u16*)p;
    p = memory::pointer_add(p, sizeof(u16));
    u32 samplerate = *(u32*)p;
    p = memory::pointer_add(p, sizeof(u32));
    p = memory::pointer_add(p, sizeof(u32)); // byterate
    p = memory::pointer_add(p, sizeof(u16)); // blockalign
    u32 bitspersample = *(u16*)p;
    p = memory::pointer_add(p, sizeof(u16));

    if (audioformat != 1 || (bitspersample != 8 && bitspersample != 16))
      return false;

    p = memory::pointer_add(block_start, subchunk1size);

    _header.sampling_rate = samplerate;
    _header.bit_depth = bitspersample;
    _header.num_channels = channels;
    _header._range = (f32)json::get_number(jsn, json::root(jsn), "range");
    _header._bus   = compiler::create_id_string(w, json::get_string(jsn, json::root(jsn), "bus"));
    _header._flags = 0;

    if (is_vorbis)
      _header._flags |= SoundResource::Flags::VORBIS;

    if (json::get_bool(jsn, json::root(jsn), "loop", false))
      _header._flags |= SoundResource::Flags::LOOP;

    if (json::get_bool(jsn, json::root(jsn), "single_instance", false))
      _header._flags |= SoundResource::Flags::SINGLE_INSTANCE;

    u32 chunk = *(u32*)p;
    p = memory::pointer_add(p, sizeof(u32));

    if (chunk == MAKEDWORD('L', 'I', 'S', 'T')) {
      u32 size = *(u32*)p;
      p = memory::pointer_add(p, sizeof(u32));
      for (u32 i = 0; i < size; i++)
        p = memory::pointer_add(p, sizeof(u8));
      chunk = *(u32*)p;
      p = memory::pointer_add(p, sizeof(u32));
    }

    if (chunk != MAKEDWORD('d', 'a', 't', 'a'))
      return false;

    const u32 subchunk2size = *(u32*)p;
    const u32 num_samples = (subchunk2size / (bitspersample / 8)) / channels;
    const f32 duration = (f32)num_samples / samplerate;

    if (duration <= in_memory_duration) {
      _header.num_samples = num_samples;
      _header.num_stream_samples = 0;
    } else {
      _header.num_samples = (u32)(in_memory_duration * num_samples / duration);
      _header.num_stream_samples = num_samples - _header.num_samples;
    }
    p = memory::pointer_add(p, sizeof(u32));

    fwrite(&_header, sizeof(SoundResource), 1, w.data);
    fwrite(p, _header.bit_depth / 8 * _header.num_channels, _header.num_samples, w.data);

    _stream_data = memory::pointer_add(p, _header.bit_depth / 8 * _header.num_channels * _header.num_samples);
    return true;
  }

  bool SoundCompiler::compile_stream(Work &w)
  {
    fwrite(_stream_data, _header.bit_depth / 8 * _header.num_channels, _header.num_stream_samples, w.data);
    return true;
  }

  bool SoundCompiler::has_stream()
  {
    return true;
  }
}