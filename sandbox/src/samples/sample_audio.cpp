#include "sample.h"

namespace
{
  using namespace pge;
  u64 toke;
}

namespace app
{
  using namespace pge;

  namespace sample_audio
  {
    void update(pge::f64 dt)
    {
      (void)dt;
      if (keyboard::pressed(KEYBOARD_KEY_P))
        audio::trigger_sound(global_game_world, "sfx/helicopter_approach");

      if (keyboard::pressed(KEYBOARD_KEY_M))
        audio::trigger_sound(global_game_world, "sfx/shell_fall");

      if (keyboard::pressed(KEYBOARD_KEY_L))
        toke = audio::trigger_sound(global_game_world, "sfx/toke_and_exhale");

      if (keyboard::pressed(KEYBOARD_KEY_K))
        audio::stop(global_game_world, toke);
    }

    void shutdown()
    {
      audio::stop_all(global_game_world);
    }
  }
}