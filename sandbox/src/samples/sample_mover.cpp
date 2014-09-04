#include <runtime/array.h>
#include <runtime/trace.h>
#include <runtime/array.h>
#include "sample.h"

#define PI 3.14159265359f

namespace app
{
  using namespace pge;

  Array<u64> *actors = NULL;
  u64  player;
  u64  player_mover;
  bool ascending;
  f32  fly_time;
  f32  fly_from;

  u64 text_cu, text_cd, text_cs;

  const u32 num_ground_vertices = 17;
  const u32 ground_size = num_ground_vertices * 2;

  const f32 SPEED = 512.f;
  const f32 ASCENT_HEIGHT   = 256.0f;
  const f32 ASCENT_DURATION = 0.2f;
  const f32 FALL_ACCELERATION_DURATION = 0.2f;
  const f32 FALL_SPEED = 2048.0f;

  char *description = "Arrows to move the avatar";

  f32 ground_vertices[ground_size] ={
    0, 450,
    200, 100,
    400, 0,
    650, 0,
    650, 50,
    700, 50,
    700, 100,
    750, 100,
    750, 150,
    850, 150,
    850, 100,
    900, 100,
    900, 50,
    950, 50,
    950, 0,
    1200, 0,
    1600, 450
  };

  inline f32 ease_out_sine(f32 t, f32 b, f32 c, f32 d) {
    return c * sin(t / d * (PI / 2)) + b;
  };

  inline f32 ease_in_sine(f32 t, f32 b, f32 c, f32 d) {
    return -c / 2 * (cos(PI*t / d) - 1) + b;
  };

  namespace sample_mover
  {
    void init()
    {
      global_sample_desciption = description;

      Allocator &a = memory_globals::default_allocator();
      actors = MAKE_NEW(a, Array<u64>, a);
      global_debug_physic = true;

      // ground
      array::push_back(*actors, world::spawn_actor_chain(global_game_world, (glm::vec2*)ground_vertices, num_ground_vertices, false, false, false, false,
        "ground", "solid", glm::vec3(-800, -400, 0), IDENTITY_ROTATION));
      // left wall
      array::push_back(*actors, world::spawn_actor_box(global_game_world, 100, 900, false, false, false, false,
        "ground", "solid", glm::vec3(-850, 50, 0), IDENTITY_ROTATION));
      // right wall
      array::push_back(*actors, world::spawn_actor_box(global_game_world, 100, 900, false, false, false, false,
        "ground", "solid", glm::vec3(850, 50, 0), IDENTITY_ROTATION));

      // left_platform
      //array::push_back(*actors, world::spawn_actor_box(global_game_world, 300, 100, true, true, false, false,
      //  "default", "solid", glm::vec3(-450, 50, 0), IDENTITY_ROTATION));

      // center_platform
      //array::push_back(*actors, world::spawn_actor_box(global_game_world, 300, 100, true, true, false, false,
      //  "default", "solid", glm::vec3(0, 50, 0), IDENTITY_ROTATION));

      // right_platform
      //array::push_back(*actors, world::spawn_actor_box(global_game_world, 300, 100, true, true, false, false,
      //  "default", "solid", glm::vec3(450, 50, 0), IDENTITY_ROTATION));

      player = world::spawn_unit(global_game_world, "units/spineboy/spineboy_mover", glm::vec3(-300, 0, 0), IDENTITY_ROTATION);
      player_mover = unit::mover(global_game_world, player, 0);

      text_cu = world::spawn_text(global_game_world, global_font_name, "collides up: 0", TEXT_ALIGN_LEFT, glm::vec3(0, 0, 0), IDENTITY_ROTATION);
      text_cd = world::spawn_text(global_game_world, global_font_name, "collides down: 0", TEXT_ALIGN_LEFT, glm::vec3(0, -20, 0), IDENTITY_ROTATION);
      text_cs = world::spawn_text(global_game_world, global_font_name, "collides sides: 0", TEXT_ALIGN_LEFT, glm::vec3(0, -40, 0), IDENTITY_ROTATION);

      ascending = false;
      fly_time  = 0;
    }

    void update(pge::f64 dt)
    {
      f32 vx, vy;
      glm::vec3 p;
      vx = 0;
      vy = 0;

      mover::get_position(global_game_world, player_mover, p);

      if (mover::collides_down(global_game_world, player_mover)) {
        if (keyboard::button(KEYBOARD_KEY_UP) && !keyboard::button(KEYBOARD_KEY_DOWN)) {
          ascending = true;
          fly_from  = p.y;
          fly_time  = 0;
        }
      }

      if (ascending) {
        // ascending
        fly_time += (f32)dt;
        vy = f32((ease_out_sine((f32)fly_time, fly_from, ASCENT_HEIGHT, ASCENT_DURATION) - p.y)/dt);
        if (fly_time >= ASCENT_DURATION) {
          ascending = false;
          fly_time  = 0;
          fly_from  = p.y + vy;
        }
      } else {
        if (mover::collides_down(global_game_world, player_mover)) {
          // grounded
          if (keyboard::button(KEYBOARD_KEY_UP) && !keyboard::button(KEYBOARD_KEY_DOWN)) {
            ascending = true;
            fly_from  = p.y;
            fly_time  = 0;
          }
        } else {
          // falling
          fly_time += (f32)dt;
          vy = fly_time < FALL_ACCELERATION_DURATION
            ? ease_in_sine((f32)fly_time, 0, FALL_SPEED, FALL_ACCELERATION_DURATION)
            : FALL_SPEED;
          vy *=-1;
        }
      }

      if (keyboard::button(KEYBOARD_KEY_RIGHT) && !keyboard::button(KEYBOARD_KEY_LEFT))
        vx = SPEED;

      if (keyboard::button(KEYBOARD_KEY_LEFT) && !keyboard::button(KEYBOARD_KEY_RIGHT))
        vx = -SPEED;

      mover::move(global_game_world, player_mover, glm::vec3(vx, vy, 0) * (f32)dt);

      if (mover::collides_up(global_game_world, player_mover))
        text::set_string(global_game_world, text_cu, "collides up: 1");
      else
        text::set_string(global_game_world, text_cu, "collides up: 0");

      if (mover::collides_down(global_game_world, player_mover))
        text::set_string(global_game_world, text_cd, "collides down: 1");
      else
        text::set_string(global_game_world, text_cd, "collides down: 0");

      if (mover::collides_sides(global_game_world, player_mover))
        text::set_string(global_game_world, text_cs, "collides side: 1");
      else
        text::set_string(global_game_world, text_cs, "collides side: 0");
    }

    void shutdown()
    {
      u64 *actor, *end = array::end(*actors);
      for (actor = array::begin(*actors); actor < end; actor++)
        world::despawn_actor(global_game_world, *actor);

      world::despawn_unit(global_game_world, player);
      world::despawn_text(global_game_world, text_cd);
      world::despawn_text(global_game_world, text_cu);
      world::despawn_text(global_game_world, text_cs);


      MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, actors);
    }
  }
}
