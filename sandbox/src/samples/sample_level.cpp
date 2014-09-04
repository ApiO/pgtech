#include "sample.h"

namespace app
{
  namespace sample_level
  {
    using namespace pge;
    u64 level;

    void init()
    {
      //global_debug_physic = true;
      //level = world::load_level(global_game_world, "levels/sample.pglevel", IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    }

    void shutdown()
    {
      //world::unload_level(global_game_world, level);
      global_debug_physic = false;
    }
  }
}