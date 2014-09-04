#pragma once

#include <data/sprite.h>
#include <engine/pge_types.h>
#include <runtime/collection_types.h>
#include <culling/culling_system.h>
#include "pose.h" // moche -> pose_types.h

namespace pge
{
  struct Sprite
  {
    Pose pose;
    const SpriteResource *resource;
    u32   group;
    u32   order;
    Color color;
    u8    blend_mode;
    u32   frame;
    bool  frame_changed;
    u64   aabb;
  };

  struct SpriteSystem
  {
    SpriteSystem(Allocator &a, CullingSystem &cs);
    IdLookupTable<Sprite> sprites;
    CullingSystem        *culling_system;
  };
}