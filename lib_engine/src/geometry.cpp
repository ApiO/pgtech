#include "application.h"
#include "geometry/geometric_system.h"
#include "pose.h"

// C++ API
namespace pge
{
  namespace geometry
  {
    // Pose stuff

    void get_world_position(u64 world, u64 geometry, glm::vec3 &v)
    {
      pose::get_world_translation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void get_world_rotation(u64 world, u64 geometry, glm::quat &q)
    {
      pose::get_world_rotation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), q);
    }

    void get_world_scale(u64 world, u64 geometry, glm::vec3 &v)
    {
      pose::get_world_scale(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void get_world_pose(u64 world, u64 geometry, glm::mat4 &m)
    {
      pose::get_world_pose(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), m);
    }


    void get_local_position(u64 world, u64 geometry, glm::vec3 &v)
    {
      pose::get_local_translation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void get_local_rotation(u64 world, u64 geometry, glm::quat &q)
    {
      pose::get_local_rotation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), q);
    }

    void get_local_scale(u64 world, u64 geometry, glm::vec3 &v)
    {
      pose::get_local_scale(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void get_local_pose(u64 world, u64 geometry, glm::mat4 &m)
    {
      compose_mat4(m,
                   geometric_system::get_pose(application::world(world).geometric_system, geometry)
                   ._local);
    }

    void set_local_position(u64 world, u64 geometry, const glm::vec3 &v)
    {
      pose::set_local_translation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void set_local_rotation(u64 world, u64 geometry, const glm::quat &q)
    {
      pose::set_local_rotation(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), q);
    }

    void set_local_scale(u64 world, u64 geometry, const glm::vec3 &v)
    {
      pose::set_local_scale(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), v);
    }

    void set_local_pose(u64 world, u64 geometry, const glm::mat4 &m)
    {
      DecomposedMatrix dm;
      decompose_mat4(m, dm);
      pose::set_local_pose(
        geometric_system::get_pose(application::world(world).geometric_system, geometry), dm);
    }
  }
}