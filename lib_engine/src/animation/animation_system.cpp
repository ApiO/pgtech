#include <runtime/idlut.h>
#include <runtime/timer.h>

#include <scene/scene_system.h>
#include <resource/unit_resource.h>

#include "animation_system.h"

#if CHRONO_STEPS
#include <utils/app_watcher.h>
#endif

namespace pge
{
  namespace animation_system
  {
    u64 play(AnimationSystem &system, Unit &unit, u32 animation, f32 from, f32 to, bool loop, f32 speed)
    {
      if (!idlut::has(system._sessions, unit->animation_session)) {
        AnimationSystem::Session ns;
        ns.unit = unit;
        ns.player_session = animation_player::create_session(system._player, unit->animation_set);
        unit->animation_session = idlut::add(system._sessions, ns);
      }

      const AnimationSystem::Session &s = *idlut::lookup(system._sessions, unit->animation_session);
      animation_player::play(system._player, s.player_session, animation, from, to, loop, speed);
      return unit->animation_session;
    }

    void update(AnimationSystem &system, f64 delta_time)
    {
#if CHRONO_STEPS
      Timer(timer);
      start_timer(timer);
#endif
      animation_player::update(system._player, delta_time);
#if CHRONO_STEPS
      app_watcher::save_value(WL_ANIMATION_UPDATE, get_elapsed_time_in_ms(timer));
      start_timer(timer);
#endif

      const IdLookupTable<AnimationSystem::Session>::Entry *e   = idlut::begin(system._sessions);
      const IdLookupTable<AnimationSystem::Session>::Entry *end = idlut::end(system._sessions);

      for (; e < end; e++) {
        if (animation_player::played(system._player, e->value.player_session)) {
          animation_player::destroy_session(system._player, e->value.player_session);
          idlut::remove(system._sessions, e->id);
          continue;
        }

        const Unit &unit = e->value.unit;
        const UnitResource::Node *nodes = unit_resource::nodes(unit->resource);
        const UnitResource::BoneTrack *bone_tracks = unit_resource::bone_tracks(unit->resource);
        const UnitResource::SpriteTrack *sprite_tracks = unit_resource::sprite_tracks(unit->resource);
        
        for (u16 i = 0; i < unit->resource->num_bone_tracks; i++) {
          glm::vec2 t(nodes[bone_tracks[i].node].pose.tx, nodes[bone_tracks[i].node].pose.ty);
          glm::vec2 s(nodes[bone_tracks[i].node].pose.sx, nodes[bone_tracks[i].node].pose.sy);
          f32 r = nodes[bone_tracks[i].node].pose.rotation;

          t += animation_player::get_bone_track_translation(unit->world->animation_system._player, e->value.player_session, bone_tracks[i].track);
          r += animation_player::get_bone_track_rotation(unit->world->animation_system._player, e->value.player_session, bone_tracks[i].track);
          s += animation_player::get_bone_track_scale(unit->world->animation_system._player, e->value.player_session, bone_tracks[i].track) - glm::vec2(1, 1);

          const Pose p(glm::vec3(t.x, t.y, 0), glm::angleAxis(r, glm::vec3(0, 0, 1)), glm::vec3(s.x, s.y, 1));
          scene_system::set_pose(unit->world->scene_system, unit->scene_graph, bone_tracks[i].node, p);
        }

        for (u16 i = 0; i < unit->resource->num_sprite_tracks; i++) {
          const u16 *sprite_track_frames = unit_resource::sprite_track_frames(unit->resource, sprite_tracks + i);
          u32 frame = animation_player::get_sprite_track_frame(unit->world->animation_system._player, e->value.player_session, sprite_tracks[i].track);

          if (frame != DEFAULT_FRAME && frame != NO_FRAME)
            unit->sprites[sprite_tracks[i].sprite]->frame = sprite_track_frames[frame];
        }
      }
#if CHRONO_STEPS
      app_watcher::save_value(WL_ANIMATION_TO_SCENE, get_elapsed_time_in_ms(timer));
#endif
    }

    void destroy_session(AnimationSystem &system, u64 session)
    {
      if (!idlut::has(system._sessions, session))
        return;

      animation_player::destroy_session(system._player, idlut::lookup(system._sessions, session)->player_session);
      idlut::remove(system._sessions, session);
    }
  }
}