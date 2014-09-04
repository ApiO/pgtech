#include <runtime/array.h>
#include <engine/pge_types.h>
#include "sample.h"

namespace
{
  using namespace pge;
  using namespace app;

  Array<u64> *units = NULL;
  f32 pad_sense = .6f;
  u64 num_balls_text;

  glm::quat rot = glm::angleAxis(glm::radians(90.f), IDENTITY_Z_ROTATION);

  const u32 num_grounds = 3;
  u64 grounds[num_grounds];
  
  inline void remove_balls()
  {
    for (u32 i = 0; i < array::size(*units); i++)
      world::despawn_unit(global_game_world, *(array::begin(*units) + i));
    array::resize(*units, 0u);
  }

  inline void spawn_balls()
  {
    u32 ct = randomize(10, 20);
    for (u32 i = 0; i < ct; i++){
      u32 r = randomize(0, 20);
      glm::vec3 pos((f32)r, global_screen.h2 + r, 0.f);
      array::push_back(*units, world::spawn_unit(global_game_world, "units/myball", pos, rot));
    }
  }



  inline void handle_keyboard()
  {
    // handles buttons
    if (keyboard::any_pressed() || keyboard::any_released())
    {
      // removes all balls
      if (keyboard::pressed(KEYBOARD_KEY_C) && array::size(*units))
        remove_balls();

      // spawn balls
      if (keyboard::pressed(KEYBOARD_KEY_SPACE))
        spawn_balls();
    }
  }

  inline void handle_pad()
  {
    if (!pad::active(global_pad_index)) return;

    // handles buttons
    if (pad::any_pressed(global_pad_index) || pad::any_released(global_pad_index))
    {
      // removes all balls
      if (pad::pressed(global_pad_index, PAD_KEY_12) && array::size(*units))
        remove_balls();
      
      // spawn balls
      if (pad::button(global_pad_index, PAD_KEY_13))
        spawn_balls();
    }
  }
}

namespace app
{
  using namespace pge;

  namespace sample_stress_my_balls
  {
    char *description = "3D navigation enable (pad, keyboard+mouse)\nSPACE: spawn balls\nC: clear balls";
    //*
    void init()
    {
      global_sample_desciption = description;
      //global_debug_physic = true;

      Allocator &a = memory_globals::default_allocator();
      units = MAKE_NEW(a, Array<u64>, a);

      glm::vec3 spawn_pos(0.f, -global_screen.h2, 0.f);
      glm::quat spawn_rot(1.f, 0.f, 0.f, 0.f);

      spawn_pos.x = 0.f;
      spawn_pos.y = -global_screen.h2 + 46.f + 10;
      grounds[0] = world::spawn_unit(global_game_world, "units/ground", spawn_pos, spawn_rot);

      spawn_pos.y -= 100.f;

      spawn_pos.x = -500.f;
      grounds[1] = world::spawn_unit(global_game_world, "units/ground", spawn_pos, glm::angleAxis(glm::radians(-45.f), IDENTITY_Z_ROTATION));

      spawn_pos.x = 500.f;
      grounds[2] = world::spawn_unit(global_game_world, "units/ground", spawn_pos, glm::angleAxis(glm::radians(45.f), IDENTITY_Z_ROTATION));

      num_balls_text = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, glm::vec3(0.f, 0.f, 0.f), IDENTITY_ROTATION);
      text::set_local_position(global_gui_world, num_balls_text, glm::vec3(-global_screen.w2 + 10, 0.f, 0.f));

      glm::vec3 psition(0, 0, 1200);
      camera_free::initialize(psition);
    }

    void update(f64 dt)
    {
      (void)dt;

      handle_pad();
      handle_keyboard();

      char value[256];
      sprintf(value, "%d balls", array::size(*units));
      text::set_string(global_gui_world, num_balls_text, value);

      camera_free::update(dt);
    }

    void shutdown()
    {
      world::despawn_text(global_gui_world, num_balls_text);

      for (u32 i = 0; i < num_grounds; i++)
        world::despawn_unit(global_game_world, grounds[i]);

      u64 *unit, *end = array::end(*units);
      for (unit = array::begin(*units); unit < end; unit++)
        world::despawn_unit(global_game_world, *unit);

      MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, units);
      camera_free::reset_ortho();
    }
  }
}