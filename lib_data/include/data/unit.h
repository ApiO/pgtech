#pragma once

#include <runtime/types.h>
#include "types.h"

namespace pge
{
  const u16 NO_FRAME = 0xFFFFu;
  const u16 DEFAULT_FRAME = 0xFFFEu;

  struct UnitResource
  {
    struct Node {
      u32 name;
      i32 parent; // -1 if root
      PoseResource pose;
    };

    struct Sprite {
      u32 node;
      u32 name;
      u32 tpl; // resource template
      PoseResource pose;
      u32 order;
      u8  color[4];
    };
    
    struct BoneTrack {
      u32 node;
      u32 track;
    };

    struct SpriteTrack {
      u32 sprite;
      u32 track;
      u32 frame_offset;
    };

    struct Actor {
      u32 instance_name;
      u32 node;
      u32 actor;
      PoseResource pose;
    };
    
    u32 num_nodes;
    u32 num_actors;
    u32 num_sprites;
    u32 num_movers;
    u16 num_bone_tracks;
    u16 num_sprite_tracks;
    u32 animation_set;
    u32 _actor_offset;
    u32 _sprite_offset;
    u32 _mover_offset;
    u32 _bone_track_offset;
    u32 _sprite_track_offset;
  };
}