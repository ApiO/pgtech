#pragma once

#include <runtime/types.h>

namespace pge
{ 
  static char *ResourceExtension[] = 
  {
    "",
    "fnt",   
    "as",
    "pgpack",
    "pgtex", 
    "pgunit",
    "pgsprite",
    "pgphys",
    "pgactor",
    "pgshape",
    "pganims",
    "pglevel",
    "pgshader",
    "pgaudio",
    "pgsound",
    "pgptcl"
  };

  static char *ResourceTypeNames[] = 
  {
    "data",
    "font",   
    "script",
    "package",
    "texture", 
    "unit",
    "sprite",
    "physics",
    "actor",
    "shape",
    "animset",
    "level",
    "shader",
    "audio",
    "sound",
    "particle"
  };

  const u32 NumResourceExtension = sizeof(ResourceExtension)/sizeof(char*);
  
  enum ResourceType
  {
    RESOURCE_TYPE_DATA = 0,
    RESOURCE_TYPE_FONT,
    RESOURCE_TYPE_SCRIPT,
    RESOURCE_TYPE_PACKAGE,
    RESOURCE_TYPE_TEXTURE,
    RESOURCE_TYPE_UNIT,
    RESOURCE_TYPE_SPRITE,
    RESOURCE_TYPE_PHYSICS,
    RESOURCE_TYPE_ACTOR,
    RESOURCE_TYPE_SHAPE,
    RESOURCE_TYPE_ANIMSET,
    RESOURCE_TYPE_LEVEL,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_AUDIO,
    RESOURCE_TYPE_SOUND,
    RESOURCE_TYPE_PARTICLE,
  };
  
  union DataId
  {
    u64   as64;
    struct {
      u32 type;
      u32 name;
    } fields;
  };

  struct PoseResource
  {
    f32 tx, ty, tz;
    f32 rx, ry, rz;
    f32 sx, sy;
  };

  enum BlendMode
  {
    BLEND_MODE_NORMAL = 0,
    BLEND_MODE_MULTIPLY,
    BLEND_MODE_SCREEN,
    BLEND_MODE_OVERLAY,
    BLEND_MODE_ADDITIVE
  };
}