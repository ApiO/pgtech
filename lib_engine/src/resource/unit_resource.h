#pragma once

#include <data/unit.h>
#include <data/mover.h>

namespace pge
{
  namespace unit_resource
  {
    u32 num_nodes(const UnitResource *unit);
    u32 num_actors(const UnitResource *unit);
    u32 num_sprites(const UnitResource *unit);
    u32 num_movers(const UnitResource *unit);
    u32 num_bone_tracks(const UnitResource *unit);
    u32 num_sprite_tracks(const UnitResource *unit);

    const UnitResource::Node *nodes(const UnitResource *unit);
    const UnitResource::Actor *actors(const UnitResource *unit);
    const UnitResource::Sprite *sprites(const UnitResource *unit);
    const MoverResource *movers(const UnitResource *unit);
    const UnitResource::BoneTrack *bone_tracks(const UnitResource *unit);
    const UnitResource::SpriteTrack *sprite_tracks(const UnitResource *unit);
    const u16 *sprite_track_frames(const UnitResource *unit, const UnitResource::SpriteTrack *track);
  }

  namespace unit_resource
  {
    inline u32 num_nodes(const UnitResource *unit)
    {
      return unit->num_nodes;
    }
    
    inline u32 num_actors(const UnitResource *unit)
    {
      return unit->num_actors;
    }

    inline u32 num_sprites(const UnitResource *unit)
    {
      return unit->num_sprites;
    }

    inline u32 num_bone_tracks(const UnitResource *unit)
    {
      return unit->num_bone_tracks;
    }

    inline u32 num_sprite_tracks(const UnitResource *unit)
    {
      return unit->num_sprite_tracks;
    }

    inline u32 num_movers(const UnitResource *unit)
    {
      return unit->num_movers;
    }


    inline const UnitResource::Node *nodes(const UnitResource *unit)
    {
      return (const UnitResource::Node*)((char*)unit + sizeof(UnitResource));
    }

    inline const UnitResource::Actor *actors(const UnitResource *unit)
    {
      return (const UnitResource::Actor*)((char*)unit + unit->_actor_offset);
    }

    inline const UnitResource::Sprite *sprites(const UnitResource *unit)
    {
      return (const UnitResource::Sprite*)((char*)unit + unit->_sprite_offset);
    }

    inline const MoverResource *movers(const UnitResource *unit)
    {
      return (const MoverResource*)((char*)unit + unit->_mover_offset);
    }

    inline const UnitResource::BoneTrack *bone_tracks(const UnitResource *unit)
    {
      return (const UnitResource::BoneTrack*)((char*)unit + unit->_bone_track_offset);
    }

    inline const UnitResource::SpriteTrack *sprite_tracks(const UnitResource *unit)
    {
      return (const UnitResource::SpriteTrack*)((char*)unit + unit->_sprite_track_offset);
    }

    inline const u16 *sprite_track_frames(const UnitResource *unit, const UnitResource::SpriteTrack *track)
    {
      return (u16*)((char*)unit + track->frame_offset);
    }
  }
}