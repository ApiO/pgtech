#include <runtime/array.h>
#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_unit
  {
    Array<u64> *units = NULL;

    void init()
    {
      u64 boy;

      glm::vec3 spawn_pos(-300.f, -300.f, 0.f);
      glm::quat spawn_rot(1.f, 0.f, 0.f, 0.f);

      Allocator &a = memory_globals::default_allocator();
      units = MAKE_NEW(a, Array<u64>, a);

      /*
      for (u32 i = 0; i < 10; i++)
      {
        spawn_pos.x+=50;
        boy = array::push_back(*units, world::spawn_unit(global_game_world, "units/spineboy/spineboy", spawn_pos, spawn_rot));
        unit::play_animation(global_game_world, boy, "walk", 0, 0, true, 4.f);
      }
      */

      spawn_pos.y =  -350;
      spawn_pos.x = - 200;
      boy = array::push_back(*units, world::spawn_unit(global_game_world, "units/spineboy/spineboy", spawn_pos, spawn_rot));
      unit::play_animation(global_game_world, boy, "walk", 0, 0, true, .25f);

      spawn_pos.x = 0;
      boy = array::push_back(*units, world::spawn_unit(global_game_world, "units/spineboy/spineboy", spawn_pos, spawn_rot));
      unit::play_animation(global_game_world, boy, "walk", 0, 0, true, 1.f);

      spawn_pos.x =  200;
      boy = array::push_back(*units, world::spawn_unit(global_game_world, "units/spineboy/spineboy", spawn_pos, spawn_rot));
      unit::play_animation(global_game_world, boy, "walk", 0, 0, true, 3.f);


      u64 dragon = array::push_back(*units, world::spawn_unit(global_game_world, "units/dragon/dragon", IDENTITY_TRANSLATION, IDENTITY_ROTATION));
      unit::play_animation(global_game_world, dragon, "flying", 0, 0, true, 1.f);
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