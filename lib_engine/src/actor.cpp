#include <runtime/types.h>
#include <runtime/idlut.h>
#include <runtime/hash.h>

#include "application.h"
#include "physics/physics_system.h"

namespace pge
{
  namespace actor
  {
    bool is_static(u64 world, u64 actor)
    {
      World &w = application::world(world);
      return physics_system::is_static(w.physics_world, actor);
    }

    bool is_dynamic(u64 world, u64 actor)
    {
      World &w = application::world(world);
      return physics_system::is_dynamic(w.physics_world, actor);
    }

    bool is_physical(u64 world, u64 actor)
    {
      World &w = application::world(world);
      return physics_system::is_physical(w.physics_world, actor);
    }

    bool is_kinematic(u64 world, u64 actor)
    {
      World &w = application::world(world);
      return physics_system::is_kinematic(w.physics_world, actor);
    }

    void get_velocity(u64 world, u64 actor, glm::vec3 &v)
    {
      World &w = application::world(world);
      physics_system::get_velocity(w.physics_world, actor, v);
    }

    void set_velocity(u64 world, u64 actor, const glm::vec3 &v)
    {
      World &w = application::world(world);
      physics_system::set_velocity(w.physics_world, actor, v);
    }

    void set_touched_callback(u64 world, u64 actor, void(*callback)(const Array<ContactPoint> &contacts))
    {
      World &w = application::world(world);
      physics_system::set_touched_callback(w.physics_world, actor, callback);
    }

    void set_untouched_callback(u64 world, u64 actor, void(*callback)(const Array<ContactPoint> &contacts))
    {
      World &w = application::world(world);
      physics_system::set_untouched_callback(w.physics_world, actor, callback);
    }

    void add_impulse(u64 world, u64 actor, const glm::vec3 &impulse)
    {
      World &w = application::world(world);
      physics_system::add_impulse(w.physics_world, actor, impulse);
    }

    void set_collision_filter(u64 world, u64 actor, const char *filter)
    {
      World &w = application::world(world);
      physics_system::set_collision_filter(w.physics_world, actor, filter);
    }

    // Pose stuff

    void get_world_position(u64 world, u64 actor, glm::vec3 &v)
    {
      pose::get_world_translation(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void get_world_rotation(u64 world, u64 actor, glm::quat &q)
    {
      pose::get_world_rotation(
        physics_system::get_pose(application::world(world).physics_world, actor), q);
    }

    void get_world_scale(u64 world, u64 actor, glm::vec3 &v)
    {
      pose::get_world_scale(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void get_world_pose(u64 world, u64 actor, glm::mat4 &m)
    {
      pose::get_world_pose(
        physics_system::get_pose(application::world(world).physics_world, actor), m);
    }


    void get_local_position(u64 world, u64 actor, glm::vec3 &v)
    {
      pose::get_local_translation(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void get_local_rotation(u64 world, u64 actor, glm::quat &q)
    {
      pose::get_local_rotation(
        physics_system::get_pose(application::world(world).physics_world, actor), q);
    }

    void get_local_scale(u64 world, u64 actor, glm::vec3 &v)
    {
      pose::get_local_scale(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void get_local_pose(u64 world, u64 actor, glm::mat4 &m)
    {
      compose_mat4(m,
                   physics_system::get_pose(application::world(world).physics_world, actor)
                   ._local);
    }

    void set_local_position(u64 world, u64 actor, const glm::vec3 &v)
    {
      pose::set_local_translation(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void set_local_rotation(u64 world, u64 actor, const glm::quat &q)
    {
      pose::set_local_rotation(
        physics_system::get_pose(application::world(world).physics_world, actor), q);
    }

    void set_local_scale(u64 world, u64 actor, const glm::vec3 &v)
    {
      pose::set_local_scale(
        physics_system::get_pose(application::world(world).physics_world, actor), v);
    }

    void set_local_pose(u64 world, u64 actor, const glm::mat4 &m)
    {
      DecomposedMatrix dm;
      decompose_mat4(m, dm);
      pose::set_local_pose(
        physics_system::get_pose(application::world(world).physics_world, actor), dm);
    }

    u64 unit(u64 world, u64 actor)
    {
      World &w = application::world(world);
      const UnitRef ur = {0};
      return hash::get(w.actor_unit, actor, ur).unit;
    }

    void set_kinematic(u64 world, u64 actor)
    {
      World &w = application::world(world);
      physics_system::set_kinematic(w.physics_world, actor, true);
    }

    void clear_kinematic(u64 world, u64 actor)
    {
      World &w = application::world(world);
      physics_system::set_kinematic(w.physics_world, actor, false);
    }
  }
}