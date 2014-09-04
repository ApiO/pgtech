#include <runtime/types.h>
#include <runtime/idlut.h>
#include <runtime/hash.h>
#include <runtime/assert.h>

#include "application.h"
#include "physics/physics_system.h"

namespace pge
{
  namespace mover
  {
    void move(u64 world, u64 mover, const glm::vec3 &delta_position)
    {
      World &w = application::world(world);
      Unit  *u = idlut::lookup(w.units, hash::get(w.mover_unit, mover)->unit);

      physics_system::move_mover(w.physics_world, mover, delta_position);
      if (u) {
        ComponentRef *cr = u->components + u->mover_offset;
        for (u32 i = 0; i < u->num_movers; i++) {
          if (cr[i].component == mover) {
            u->mover = i;
            return;
          }
        }
      }
    }

    void get_position(u64 world, u64 mover, glm::vec3 &position)
    {
      World &w = application::world(world);
      physics_system::get_mover_position(w.physics_world, mover, position);
    }

    void set_position(u64 world, u64 mover, const glm::vec3 &position)
    {
      World &w = application::world(world);
      Unit  *u = idlut::lookup(w.units, hash::get(w.mover_unit, mover)->unit);

      physics_system::teleport_mover(w.physics_world, mover, position);
      if (u) {
        ComponentRef *cr = u->components + u->mover_offset;
        for (u32 i = 0; i < u->num_movers; i++) {
          if (cr[i].component == mover) {
            u->mover = i;
            return;
          }
        }
      }
    }

    u64 get_unit(u64 world, u64 mover)
    {
      World &w = application::world(world);
      return hash::get(w.mover_unit, mover)->unit;
    }

    bool collides_down(u64 world, u64 mover)
    {
      World &w = application::world(world);
      return idlut::lookup(w.physics_world.movers, mover)->collides_down;
    }

    bool collides_up(u64 world, u64 mover)
    {
      World &w = application::world(world);
      return idlut::lookup(w.physics_world.movers, mover)->collides_up;
    }

    bool collides_sides(u64 world, u64 mover)
    {
      World &w = application::world(world);
      Mover &m = *idlut::lookup(w.physics_world.movers, mover);
      return m.collides_left || m.collides_right;
    }
    bool collides_left(u64 world, u64 mover)
    {
      World &w = application::world(world);
      return idlut::lookup(w.physics_world.movers, mover)->collides_left;
    }

    bool collides_right(u64 world, u64 mover)
    {
      World &w = application::world(world);
      return idlut::lookup(w.physics_world.movers, mover)->collides_right;
    }

    void set_collision_filter(u64 world, u64 mover, const char *filter)
    {
      World &w = application::world(world);
      physics_system::set_mover_collision_filter(w.physics_world, mover, filter);
    }
  }
}
