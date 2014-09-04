#include <runtime/array.h>
#include "sample.h"

namespace app
{
  namespace sample_default
  {
    using namespace pge;
    u64 impact;

    void init()
    {
      impact = world::spawn_unit(global_game_world, "fx/fxImpact00/fxImpact00", IDENTITY_TRANSLATION, IDENTITY_ROTATION, IDENTITY_SCALE);

      unit::play_animation(global_game_world, impact, "impact", 0, 0, true, 1.f);
    }

    void update(f64 dt)
    {
      (void)dt;
    }

    void synchronize()
    {

    }

    void shutdown()
    {
      world::despawn_unit(global_game_world, impact);
    }
  }
}