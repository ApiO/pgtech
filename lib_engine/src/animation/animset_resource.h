#pragma once

#include <runtime/types.h>
#include <runtime/assert.h>

#include <data/animset.h>

namespace pge
{
  namespace animset_resource
  {
    TrackType track_index_to_type(const AnimsetResource *animset, u16 track_index, u32 *typed_index);
    u16 typed_to_track_index(const AnimsetResource *animset, TrackType type, u32 typed_index);
    u32 track_index_to_event_name(const AnimsetResource *animset, u32 track_index);

    f32   duration(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation);
    u32   num_translation_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    u32   num_rotation_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    u32   num_scale_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    u32   num_frame_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    u32   num_color_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    u32   num_event_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index);
    void  read_stream(void *dst, u32 size, void **stream);
    void  peek_stream(void *dst, u32 size, void **stream);

    const  AnimsetResource::AnimationHeader *animation(const AnimsetResource *animset, u32 animation);
    const  void *stream(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation);
  }

  namespace animset_resource
  {
    inline TrackType track_index_to_type(const AnimsetResource *animset, u16 track_index, u32 *typed_index)
    {
      if (track_index < animset->num_bone_tracks) {
        *typed_index = track_index;
        return TRACK_TYPE_BONE;
      } else if (track_index < animset->num_bone_tracks + animset->num_sprite_tracks) {
        *typed_index = track_index - animset->num_bone_tracks;
        return TRACK_TYPE_SPRITE;
      } else {
        *typed_index = track_index - (animset->num_bone_tracks + animset->num_sprite_tracks);
        return TRACK_TYPE_EVENT;
      }
    }

    inline u16 typed_to_track_index(const AnimsetResource *animset, TrackType type, u32 typed_index)
    {
      switch (type) {
      case TRACK_TYPE_BONE:   return (u16)typed_index; break;
      case TRACK_TYPE_SPRITE: return (u16)typed_index + animset->num_bone_tracks; break;
      case TRACK_TYPE_EVENT:  return (u16)typed_index + animset->num_bone_tracks + animset->num_sprite_tracks; break;
      }

      return 0;
    }

    inline const AnimsetResource::AnimationHeader *animation(const AnimsetResource *animset, u32 animation)
    {
      AnimsetResource::AnimationHeader *h = (AnimsetResource::AnimationHeader *)
        (((u8*)animset)
        + sizeof(AnimsetResource)
        +(animset->num_bone_tracks * sizeof(AnimsetResource::BoneTrack))
        + (animset->num_sprite_tracks * sizeof(AnimsetResource::SpriteTrack))
        + (animset->num_event_tracks * sizeof(AnimsetResource::EventTrack))
        );

      for (u16 i = 0; i < animset->num_animations; i++)
      if (h[i].name == animation) return &h[i];

      XERROR("could not find the animation %u.", animation);
      return NULL;
    }

    inline void peek_stream(void *dst, u32 size, void **stream)
    {
      memcpy(dst, *stream, size);
    }

    inline void read_stream(void *dst, u32 size, const void **stream)
    {
      memcpy(dst, *stream, size);
      *stream = (u8*)*stream + size;
    }

    inline f32 duration(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation) {
      return *(f32*)(((u8*)animset) + animation->data_offset);
    }

    inline u32 num_translation_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index);
    }

    inline u32 num_rotation_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index
               + animset->num_bone_tracks);
    }

    inline u32 num_scale_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index
               + animset->num_bone_tracks * 2);
    }

    inline u32 num_frame_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index
               + animset->num_bone_tracks * 3);
    }

    inline u32 num_color_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index
               + animset->num_bone_tracks * 3 + animset->num_sprite_tracks);
    }

    inline u32 num_event_keys(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation, u32 track_index) {
      return *(((u32*)(((u8*)animset) + animation->data_offset + 4)) + track_index
               + animset->num_bone_tracks * 3 + animset->num_sprite_tracks * 2);
    }

    inline const void *stream(const AnimsetResource *animset, const AnimsetResource::AnimationHeader *animation) {
      return ((u32*)((u8*)animset + animation->data_offset + 4))
        + animset->num_bone_tracks * 3 + animset->num_sprite_tracks * 2 + animset->num_event_tracks;
    }
  }
}