#pragma once

#include <glm/glm.hpp>
#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <data/animset.h>

#include "animation_types.h"

namespace pge
{
  namespace animation_player
  {
    u64  create_session(AnimationPlayer &system, const AnimsetResource *set);
    void play(AnimationPlayer &system, u64 session, u32 anim_name, f32 from, f32 to, bool loop, f32 speed);
    bool played(AnimationPlayer &system, u64 session);
    void update(AnimationPlayer &system, f64 delta_time);
    u32  playing_animation(AnimationPlayer &system, u64 session);
    glm::vec2 get_bone_track_translation(AnimationPlayer &system, u64 session, u32 track_index);
    f32 get_bone_track_rotation(AnimationPlayer &system, u64 session, u32 track_index);
    glm::vec2 get_bone_track_scale(AnimationPlayer &system, u64 session, u32 track_index);
    u32  get_sprite_track_frame(AnimationPlayer &system, u64 session, u32 track_index);
    glm::vec4 get_sprite_track_color(AnimationPlayer &system, u64 session, u32 track_index);
    void get_events(AnimationPlayer &system, u64 session, Array<u32> &events);
    void destroy_session(AnimationPlayer &system, u64 session);
  }
}

// inline implementations

namespace pge
{
  inline AnimationPlayer::AnimationPlayer(Allocator &a) :
    _sessions(a) {};

}