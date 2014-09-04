#include <runtime/array.h>
#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_physics
  {
    Array<u64> *units = NULL;

    void init()
    {
      f32 pad = 0.f;
      i32 ct  = 100;
      f32 r   = 43.f;
      f32 y_start     = global_screen.height*.66f;
      i32 ball_margin = 20;

      glm::vec3 spawn_pos = IDENTITY_TRANSLATION;
      glm::quat spawn_rot  = IDENTITY_ROTATION;

      //global_debug_physic = true;

      Allocator &a = memory_globals::default_allocator();
      units = MAKE_NEW(a, Array<u64>, a);

      for (i32 i = 0; i < ct; i++)
      {
        pad += r * 2 + ball_margin;
        spawn_pos = glm::vec3(i % 2 ? -10.f : 10.f, y_start + pad, 0.f);
        array::push_back(*units, world::spawn_unit(global_game_world, "units/myball", spawn_pos, spawn_rot));
      }

      spawn_pos.x = 0.f;
      spawn_pos.y = -global_screen.h2 + 46.f + 10;
      array::push_back(*units, world::spawn_unit(global_game_world, "units/ground", spawn_pos, spawn_rot));

      spawn_pos.y -= 100.f;

      spawn_pos.x = -500.f;
      array::push_back(*units, world::spawn_unit(global_game_world, "units/ground", spawn_pos, glm::angleAxis(glm::radians(-45.f), IDENTITY_Z_ROTATION)));

      spawn_pos.x = 500.f;
      array::push_back(*units, world::spawn_unit(global_game_world, "units/ground", spawn_pos, glm::angleAxis(glm::radians(45.f), IDENTITY_Z_ROTATION)));
    }

    void shutdown()
    {
      u64 *unit, *end = array::end(*units);
      for (unit = array::begin(*units); unit < end; unit++)
        world::despawn_unit(global_game_world, *unit);

      MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, units);
    }
  }
}