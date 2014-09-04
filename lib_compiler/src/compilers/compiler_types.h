#pragma once

#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/collection_types.h>
#include <data/sound.h>

#include <types.h>

namespace pge
{
  struct CompilerBase
  {
    CompilerBase(Allocator &_a, ResourceType _type);
    virtual bool  compile(Work &w) = 0;
    virtual bool  compile_stream(Work &w);
    virtual bool  has_stream();
    Allocator    *a;
    ResourceType  type;
  };

  struct BinaryCompiler : CompilerBase
  {
    BinaryCompiler(Allocator &a, ResourceType _type);
    virtual bool compile(Work &w) = 0;
    string_stream::Buffer buf;
    bool load_bytes(Work &w);
  };

  struct JsonCompiler : CompilerBase
  {
    JsonCompiler(Allocator &a, ResourceType _type, StringPool &sp);
    virtual bool compile(Work &w) = 0;
    Json jsn;
    bool load_json(Work &w);
  };

  struct ActorCompiler : JsonCompiler
  {
    ActorCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct AnimsetCompiler : JsonCompiler
  {
    AnimsetCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };


  struct LevelCompiler : JsonCompiler
  {
    LevelCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct PackageCompiler : JsonCompiler
  {
    PackageCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct PhysicsCompiler : JsonCompiler
  {
    PhysicsCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct ShaderCompiler : JsonCompiler
  {
    ShaderCompiler(Allocator &a, StringPool &sp);
    ~ShaderCompiler();
    bool compile(Work &w);
    void *window;
  };

  struct ShapeCompiler : JsonCompiler
  {
    ShapeCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct SpriteCompiler : JsonCompiler
  {
    SpriteCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct TextureCompiler : JsonCompiler
  {
    TextureCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct UnitCompiler : JsonCompiler
  {
    UnitCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct ScriptCompiler : BinaryCompiler
  {
    ScriptCompiler(Allocator &a);;
    bool compile(Work &w);
  };

  struct FontCompiler : BinaryCompiler
  {
    FontCompiler(Allocator &a);
    bool compile(Work &w);
  };

  struct AudioCompiler : JsonCompiler
  {
    AudioCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };

  struct SoundCompiler : JsonCompiler
  {
    SoundCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
    bool compile_stream(Work &w);
    bool has_stream();

    string_stream::Buffer _buf;
    SoundResource         _header;
    void                 *_stream_data;
  };

  struct ParticleCompiler : JsonCompiler
  {
    ParticleCompiler(Allocator &a, StringPool &sp);
    bool compile(Work &w);
  };
}