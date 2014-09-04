#include <runtime/types.h>
#include <runtime/idlut.h>

#include "application.h"
#include "physics/physics_debug.h"
#include "physics/physics_system.h"

namespace pge
{
  namespace physics
  {
    u64 create_raycast(u64 world, RaycastCallback callback, const char *filter, bool closest, bool any)
    {
      World &w = application::world(world);
      return physics_system::create_raycast(w.physics_world, callback, closest, any, filter);
    }

    void cast_raycast(u64 world, u64 raycast, const glm::vec3 &from, const glm::vec3 &to)
    {
      World &w = application::world(world);
      physics_system::cast_raycast(w.physics_world, raycast, from, to);
    }

    void destroy_raycast(u64 world, u64 raycast)
    {
      World &w = application::world(world);
      physics_system::destroy_raycast(w.physics_world, raycast);
    }

    void show_debug(bool value)
    {
      physics_debug::show(value);
    }
  }
}