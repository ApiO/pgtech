#include <runtime/array.h>
#include <runtime/trace.h>
#include "sample.h"

namespace app
{
  namespace sample_particles
  {
    using namespace pge;

    u64 effect;
    u64 np_text;

    void init()
    {
      effect = world::spawn_particles(global_game_world, "particles/potatoe_fire", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      np_text = world::spawn_text(global_gui_world, global_font_name, "", TEXT_ALIGN_LEFT, glm::vec3(-300, -200, 0), IDENTITY_ROTATION);
    }

    void update(f64 dt)
    {
      char buf[512];
      sprintf(buf, "%d", world::num_particles(global_game_world, effect));
      text::set_string(global_gui_world, np_text, buf);

      glm::vec2 mp = mouse::get_position();
      mp.x -= global_screen.width  / 2;
      mp.y -= global_screen.height / 2;
      mp.y *= -1;

      //world::move_particles(global_game_world, effect, glm::vec3(mp, 0), IDENTITY_ROTATION);
    }

    void shutdown()
    {
      world::despawn_particles(global_game_world, effect);
      world::despawn_text(global_gui_world, np_text);
    }
  }
}