#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_mr_patate
  {
    u64 mr_patate = NULL;

    void init()
    {
      mr_patate = world::spawn_unit(global_game_world, "units/mr_patate", glm::vec3(0, 0, 0), IDENTITY_ROTATION);
    }

    void shutdown()
    {
      world::despawn_unit(global_game_world, mr_patate);
    }
  }
}