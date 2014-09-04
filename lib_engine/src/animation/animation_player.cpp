#include <runtime/idlut.h>

#include "animation_player.h"
#include "animset_resource.h"
#include "glm/ext.hpp"
#include <data/unit.h> // a virer

#define AP AnimationPlayer

namespace
{
  using namespace pge;

  const float CURVE_LINEAR = 0;
  const float CURVE_STEPPED = -1;
  static const int CURVE_SEGMENTS = 10;

  template<typename T>
  void set_point_curve(AP::CurvePoint<T> &c, BezierCurve &src)
  {
    if (src.cx1 == CURVE_TYPE_LINEAR) {
      c.dfx = CURVE_LINEAR;
      return;
    }

    if (src.cx1 == CURVE_TYPE_STEPPED) {
      c.dfx = CURVE_STEPPED;
      return;
    }

    const f32 subdiv_step = 1.0f / CURVE_SEGMENTS;
    const f32 subdiv_step2 = subdiv_step * subdiv_step;
    const f32 subdiv_step3 = subdiv_step2 * subdiv_step;
    const f32 pre1 = 3 * subdiv_step;
    const f32 pre2 = 3 * subdiv_step2;
    const f32 pre4 = 6 * subdiv_step2;
    const f32 pre5 = 6 * subdiv_step3;
    const f32 tmp1x = -src.cx1 * 2 + src.cx2;
    const f32 tmp1y = -src.cy1 * 2 + src.cy2;
    const f32 tmp2x = (src.cx1 - src.cx2) * 3 + 1;
    const f32 tmp2y = (src.cy1 - src.cy2) * 3 + 1;

    c.dfx   = src.cx1 * pre1 + tmp1x * pre2 + tmp2x * subdiv_step3;
    c.dfy   = src.cy1 * pre1 + tmp1y * pre2 + tmp2y * subdiv_step3;
    c.ddfx  = tmp1x * pre4 + tmp2x * pre5;
    c.ddfy  = tmp1y * pre4 + tmp2y * pre5;
    c.dddfx = tmp2x * pre5;
    c.dddfy = tmp2y * pre5;
  }



  template<typename T>
  void evaluate(AP::CurvePoint<T> *points, u32 num_points, f64 time)
  {
    f32 x, y, dfx, dfy, ddfx, ddfy, dddfx, dddfy;

    const AP::CurvePoint<T> *end = points + num_points;
    for (AP::CurvePoint<T> *p = points; p < end; p++) {
      dfx = p->dfx;

      f32 percent = p->end_time <= p->start_time ? 0 : ((f32)time - p->start_time) / (p->end_time - p->start_time);
      percent = (percent < 0) ? 0 : (percent > 1 ? 1 : percent);

      if (dfx == CURVE_LINEAR) {
        p->value = glm::mix(p->start_value, p->end_value, percent);
        continue;
      }

      if (dfx == CURVE_STEPPED) {
        p->value = p->start_value;
        continue;
      }

      dfy   = p->dfy;
      ddfx  = p->ddfx;
      ddfy  = p->ddfy;
      dddfx = p->dddfx;
      dddfy = p->dddfy;

      x = dfx, y = dfy;
      f32 curve_percent;

      for (i32 i = CURVE_SEGMENTS - 2; i >= 0; i--) {
        if (x >= percent) {
          f32 lastX = x - dfx;
          f32 lastY = y - dfy;
          curve_percent = lastY + (y - lastY) * (percent - lastX) / (x - lastX);
          break;
        }
        if (i == 0) break;
        dfx += ddfx;
        dfy += ddfy;
        ddfx += dddfx;
        ddfy += dddfy;
        x += dfx;
        y += dfy;
      }

      if (x < percent)
        curve_percent = y + (1 - y) * (percent - x) / (1 - x); /* Last point is 1,1. */
      p->value = glm::mix(p->start_value, p->end_value, curve_percent);
    }
  }

  void inline read_curve(BezierCurve &c, CurveType type, const void **playhead)
  {
    c ={ 0 };
    if (type == CURVE_TYPE_LINEAR || type == CURVE_TYPE_STEPPED) {
      c.cx1 = (f32)type;
      return;
    }
    animset_resource::read_stream(&c, sizeof(BezierCurve), playhead);
  }

  void fetch_playhead(AP::Session &s, u32 num_fetches)
  {
    AnimsetResource::KeyframeHeader kf;
    TrackType track_type;
    u32 typed_index;

    u8 channel_and_curve;
    CurveType curve_type;
    u32 curve_index;
    f32 value[2];
    u16 channel_and_frame_or_curve;
    glm::u8vec4 tmp_color;

    for (u32 i = 0; i < num_fetches; i++) {
      animset_resource::read_stream(&kf, 6, &s.playhead);
      track_type = animset_resource::track_index_to_type(s.set, kf.track_index, &typed_index);

      switch (track_type) {
      case TRACK_TYPE_BONE:
        animset_resource::read_stream(&channel_and_curve, sizeof(u8), &s.playhead);
        curve_type  = (CurveType)(channel_and_curve >> 4);
        curve_index = typed_index * 3;

        switch ((ChannelType)(channel_and_curve & 0xF)) {
        case CHANNEL_TYPE_TRANSLATION:
          animset_resource::read_stream(value, sizeof(f32)* 2, &s.playhead);
          s.active_translation_points[typed_index].end_value   = glm::vec2(value[0], value[1]);
          s.active_translation_points[typed_index].end_time    = kf.time;
          read_curve(s.bone_tracks[typed_index].next_translation_curve, curve_type, &s.playhead);
          break;
        case CHANNEL_TYPE_ROTATION:
          animset_resource::read_stream(value, sizeof(f32), &s.playhead);
          s.active_rotation_points[typed_index].end_value   = value[0];
          s.active_rotation_points[typed_index].end_time    = kf.time;
          read_curve(s.bone_tracks[typed_index].next_rotation_curve, curve_type, &s.playhead);
          break;
        case CHANNEL_TYPE_SCALE:
          animset_resource::read_stream(value, sizeof(f32)* 2, &s.playhead);
          s.active_scale_points[typed_index].end_value   = glm::vec2(value[0], value[1]);
          s.active_scale_points[typed_index].end_time    = kf.time;
          read_curve(s.bone_tracks[typed_index].next_scale_curve, curve_type, &s.playhead);
          break;
        }
        break;
      case TRACK_TYPE_SPRITE:
        animset_resource::read_stream(&channel_and_frame_or_curve, sizeof(u16), &s.playhead);
        switch ((ChannelType)(channel_and_frame_or_curve & 0x3)) {
        case CHANNEL_TYPE_FRAME:
          s.sprite_tracks[typed_index].next_frame = channel_and_frame_or_curve >> 4;
          s.sprite_tracks[typed_index].next_frame_time = kf.time;
          break;
        case CHANNEL_TYPE_COLOR:
          s.active_color_points[typed_index].start_value = s.active_color_points[typed_index].end_value;
          s.active_color_points[typed_index].start_time  = s.active_color_points[typed_index].end_time;
          s.active_color_points[typed_index].end_time    = kf.time;

          animset_resource::read_stream(&tmp_color, sizeof(glm::u8vec4), &s.playhead);
          s.active_color_points[typed_index].end_value = glm::vec4(tmp_color);

          set_point_curve(s.active_color_points[typed_index], s.bone_tracks[typed_index].next_scale_curve);
          read_curve(s.sprite_tracks[typed_index].next_color_curve, (CurveType)(channel_and_frame_or_curve << 2), &s.playhead);
          break;
        }
        break;
      case TRACK_TYPE_EVENT:
        s.event_tracks[typed_index].time = kf.time;
        break;
      }
    }
  }

  void advance_playhead(AP::Session &s, f64 delta_time) {
    u32 num_fetches;
    s.time += delta_time * s.speed;

    do {
      num_fetches = 0;

      for (u32 i = 0; i < s.set->num_bone_tracks; i++) {
        if (s.active_translation_points[i].end_time <= s.time
            && s.bone_tracks[i].num_translation_fetches < animset_resource::num_translation_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.bone_tracks[i].num_translation_fetches;

          s.active_translation_points[i].start_value = s.active_translation_points[i].end_value;
          s.active_translation_points[i].start_time  = s.active_translation_points[i].end_time;
          set_point_curve(s.active_translation_points[i], s.bone_tracks[i].next_translation_curve);
        }

        if (s.active_rotation_points[i].end_time <= s.time
            && s.bone_tracks[i].num_rotation_fetches < animset_resource::num_rotation_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.bone_tracks[i].num_rotation_fetches;

          s.active_rotation_points[i].start_value = s.active_rotation_points[i].end_value;
          s.active_rotation_points[i].start_time  = s.active_rotation_points[i].end_time;
          set_point_curve(s.active_rotation_points[i], s.bone_tracks[i].next_rotation_curve);
        }

        if (s.active_scale_points[i].end_time <= s.time
            && s.bone_tracks[i].num_scale_fetches < animset_resource::num_scale_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.bone_tracks[i].num_scale_fetches;

          s.active_scale_points[i].start_value = s.active_scale_points[i].end_value;
          s.active_scale_points[i].start_time  = s.active_scale_points[i].end_time;
          set_point_curve(s.active_scale_points[i], s.bone_tracks[i].next_scale_curve);
        }
      }

      for (u32 i = 0; i < s.set->num_sprite_tracks; i++) {
        if (s.active_color_points[i].end_time <= s.time
            && s.sprite_tracks[i].num_color_fetches < animset_resource::num_color_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.sprite_tracks[i].num_color_fetches;

          s.active_color_points[i].start_value = s.active_color_points[i].end_value;
          s.active_color_points[i].start_time  = s.active_color_points[i].end_time;
          set_point_curve(s.active_color_points[i], s.sprite_tracks[i].next_color_curve);
        }

        if (s.sprite_tracks[i].next_frame_time <= s.time
            && s.sprite_tracks[i].num_frame_fetches < animset_resource::num_frame_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.sprite_tracks[i].num_frame_fetches;
          s.sprite_tracks[i].frame = s.sprite_tracks[i].next_frame;
        }
      }

      for (u32 i = 0; i < s.set->num_event_tracks; i++) {
        if (s.event_tracks[i].time <= s.time
            && s.event_tracks[i].num_fetches < animset_resource::num_event_keys(s.set, s.animation_header, i)) {
          ++num_fetches;
          ++s.event_tracks[i].num_fetches;
          array::push_back(*s.events, animset_resource::typed_to_track_index(s.set, TRACK_TYPE_EVENT, i));
        }
      }

      fetch_playhead(s, num_fetches);
    } while (num_fetches > 0);
  }
}

namespace pge
{
  namespace animation_player
  {
    u64 create_session(AnimationPlayer &player, const AnimsetResource *set)
    {
      //AP::Session s(player._sessions._data._allocator);
      AP::Session s;
      s.set = set;
      s.buf_size = 0
        + sizeof(AP::BoneTrack)               * set->num_bone_tracks
        + sizeof(AP::SpriteTrack)             * set->num_sprite_tracks
        + sizeof(AP::EventTrack)              * set->num_event_tracks
        + sizeof(AP::CurvePoint<glm::vec2>)   * set->num_bone_tracks   // translation points
        + sizeof(AP::CurvePoint<glm::vec2>)   * set->num_bone_tracks   // scale points
        + sizeof(AP::CurvePoint<f32>)         * set->num_bone_tracks   // rotation points
        + sizeof(AP::CurvePoint<glm::vec4>)   * set->num_sprite_tracks // color points
        + sizeof(Array<u16>);                                          // events
      const void *buffer = player._sessions._data._allocator->allocate(s.buf_size);

      s.time = .0f;
      s.bone_tracks               = (AP::BoneTrack*)             (buffer);
      s.sprite_tracks             = (AP::SpriteTrack*)           (s.bone_tracks + set->num_bone_tracks);
      s.event_tracks              = (AP::EventTrack*)            (s.sprite_tracks + set->num_sprite_tracks);
      s.active_translation_points = (AP::CurvePoint<glm::vec2>*) (s.event_tracks + set->num_event_tracks);
      s.active_scale_points       = (AP::CurvePoint<glm::vec2>*) (s.active_translation_points + set->num_bone_tracks);
      s.active_rotation_points    = (AP::CurvePoint<f32>*)       (s.active_scale_points + set->num_bone_tracks);
      s.active_color_points       = (AP::CurvePoint<glm::vec4>*) (s.active_rotation_points + set->num_bone_tracks);
      s.events = new(s.active_color_points + set->num_sprite_tracks) Array<u16>(*player._sessions._data._allocator);
      s.played = false;

      return idlut::add(player._sessions, s);
    }

    void play(AnimationPlayer &player, u64 session, u32 anim_name, f32 from, f32 to, bool loop, f32 speed)
    {
      AP::Session &s = *idlut::lookup(player._sessions, session);

      // save parameters for looping
      s.from = from;
      s.animation_header = animset_resource::animation(s.set, anim_name);
      s.to   = to != 0.0f ? to : animset_resource::duration(s.set, s.animation_header);
      s.loop = loop;
      s.speed = speed;
      s.played = false;
      s.time = from;

      s.animation_stream = animset_resource::stream(s.set, s.animation_header);
      memset(s.bone_tracks, 0, s.buf_size - sizeof(Array<u16>));

      for (u32 i = 0; i < s.set->num_bone_tracks; i++)
        s.active_scale_points[i].start_value = s.active_scale_points[i].end_value = glm::vec2(1, 1);

      for (u32 i = 0; i < s.set->num_sprite_tracks; i++)
        s.sprite_tracks[i].frame = s.sprite_tracks[i].next_frame = DEFAULT_FRAME;

      s.playhead = s.animation_stream;
    }

    bool played(AnimationPlayer &player, u64 session)
    {
      const AnimationPlayer::Session *s = idlut::lookup(player._sessions, session);
      return s->played;
    }

    void update(AnimationPlayer &player, f64 delta_time)
    {
      IdLookupTable<AP::Session>::Entry *e   = idlut::begin(player._sessions);
      IdLookupTable<AP::Session>::Entry *end = idlut::end(player._sessions);

      for (; e < end; e++) {
        AP::Session &s = e->value;
        array::clear(*s.events);

        // advance playhead (update frames, events and active curve points)
        advance_playhead(s, delta_time);

        if (s.time >= s.to) { // if time is greater than the duration
          if (s.loop) { // if loop is true, replay the animation
            f64 offset = s.time - s.to;
            while (offset > s.to) offset -= s.to;
            play(player, e->id, s.animation_header->name, s.from, s.to, s.loop, s.speed);
            advance_playhead(s, offset);
          } else { // else skip evaluation
            s.played = true;
            continue;
          }
        }

        // evaluate curve values
        evaluate(s.active_translation_points, s.set->num_bone_tracks, s.time);
        evaluate(s.active_rotation_points, s.set->num_bone_tracks, s.time);
        evaluate(s.active_scale_points, s.set->num_bone_tracks, s.time);
        evaluate(s.active_color_points, s.set->num_sprite_tracks, s.time);
      }
    }

    void destroy_session(AnimationPlayer &player, u64 session)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      s->events->~Array();
      player._sessions._data._allocator->deallocate(s->bone_tracks);
      idlut::remove(player._sessions, session);
    }

    glm::vec2 get_bone_track_translation(AnimationPlayer &player, u64 session, u32 track_index)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      return s->active_translation_points[track_index].value;
    }

    f32 get_bone_track_rotation(AnimationPlayer &player, u64 session, u32 track_index)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      f32 rotation = s->active_rotation_points[track_index].value;
      while (rotation > 180)
        rotation -= 360;
      while (rotation < -180)
        rotation += 360;
      return rotation;
    }

    glm::vec2 get_bone_track_scale(AnimationPlayer &player, u64 session, u32 track_index)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      return s->active_scale_points[track_index].value;
    }

    u32 get_sprite_track_frame(AnimationPlayer &player, u64 session, u32 track_index)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      return s->sprite_tracks[track_index].frame;
    }

    glm::vec4 get_sprite_track_color(AnimationPlayer &player, u64 session, u32 track_index)
    {
      const AP::Session *s = idlut::lookup(player._sessions, session);
      return s->active_color_points[track_index].value;
    }

    u32 playing_animation(AnimationPlayer &player, u64 session) {
      return idlut::lookup(player._sessions, session)->animation_header->name;
    }
  }
}
