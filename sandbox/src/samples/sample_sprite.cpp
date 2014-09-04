#include <runtime/array.h>
#include <runtime/trace.h>
#include <runtime/array.h>
#include "sample.h"

namespace app
{
  namespace sample_sprite
  {
    using namespace pge;

    const f64 COOLDOWN = 0.5;
    f64 timer = COOLDOWN;
    Array<u64> *sprites;

    void init()
    {
      Allocator &a = memory_globals::default_allocator();
      sprites = MAKE_NEW(a, Array<u64>, a);

      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/dragon/sprites/R_wing", glm::vec3(-600, 0, 0), glm::quat(1,0,0,0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/dragon/sprites/L_wing", glm::vec3(-100, 0, 0), glm::quat(1,0,0,0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/spineboy/sprites/eyes", glm::vec3(250, 0, 0), glm::quat(1,0,0,0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/spineboy/sprites/head", glm::vec3(300, 0, 0), glm::quat(1,0,0,0)));
    }

    void update(f64 dt)
    {
      timer -= dt;

      if (timer >= 0)
        return;

      timer = COOLDOWN;
      for (u32 i = 0; i < array::size(*sprites); i++) {
        i32 cf = sprite::get_current_frame(global_game_world, (*sprites)[i]);
        if (cf < sprite::get_num_frames(global_game_world, (*sprites)[i]) - 1)
          sprite::set_frame(global_game_world, (*sprites)[i], cf+1);
        else
          sprite::set_frame(global_game_world, (*sprites)[i], 0);
      }
    }

    void synchronize()
    {

    }

    void shutdown()
    {
      Allocator &a = memory_globals::default_allocator();

      for (u32 i = 0; i < array::size(*sprites); i++)
        world::despawn_sprite(global_game_world, (*sprites)[i]);

      MAKE_DELETE(a, Array<u64>, sprites);
    }
  }
}