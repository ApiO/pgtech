#include "sprite/sprite_system.h"
#include "runtime/hash.h"
#include "application.h"

namespace pge
{
  namespace sprite
  {
    i32 get_frame(u64 world, u64 sprite, const char *name)
    {
      return sprite_system::get_frame(application::world(world).sprite_system, sprite, name);
    }

    i32 get_setup_frame(u64 world, u64 sprite)
    {
      return sprite_system::get_setup_frame(application::world(world).sprite_system, sprite);
    }

    i32 get_current_frame(u64 world, u64 sprite)
    {
      return sprite_system::get_current_frame(application::world(world).sprite_system, sprite);
    }

    i32 get_num_frames(u64 world, u64 sprite)
    {
      return sprite_system::get_num_frames(application::world(world).sprite_system, sprite);
    }

    void get_size(u64 world, u64 sprite, glm::vec2 &size)
    {
      sprite_system::get_size(application::world(world).sprite_system, sprite, size);
    }

    void set_frame(u64 world, u64 sprite, i32 frame)
    {
      return sprite_system::set_frame(application::world(world).sprite_system, sprite, frame);
    }

    // Pose stuff    

    void get_world_position(u64 world, u64 sprite, glm::vec3 &v)
    {
      pose::get_world_translation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void get_world_rotation(u64 world, u64 sprite, glm::quat &q)
    {
      pose::get_world_rotation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), q);
    }

    void get_world_scale(u64 world, u64 sprite, glm::vec3 &v)
    {
      pose::get_world_scale(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void get_world_pose(u64 world, u64 sprite, glm::mat4 &m)
    {
      pose::get_world_pose(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), m);
    }


    void get_local_position(u64 world, u64 sprite, glm::vec3 &v)
    {
      pose::get_local_translation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void get_local_rotation(u64 world, u64 sprite, glm::quat &q)
    {
      pose::get_local_rotation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), q);
    }

    void get_local_scale(u64 world, u64 sprite, glm::vec3 &v)
    {
      pose::get_local_scale(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void get_local_pose(u64 world, u64 sprite, glm::mat4 &m)
    {
      compose_mat4(m, sprite_system::get_pose(application::world(world).sprite_system, sprite)._local);
    }

    void set_local_position(u64 world, u64 sprite, const glm::vec3 &v)
    {
      pose::set_local_translation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void set_local_rotation(u64 world, u64 sprite, const glm::quat &q)
    {
      pose::set_local_rotation(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), q);
    }

    void set_local_scale(u64 world, u64 sprite, const glm::vec3 &v)
    {
      pose::set_local_scale(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), v);
    }

    void set_local_pose(u64 world, u64 sprite, const glm::mat4 &m)
    {
      DecomposedMatrix dm;
      decompose_mat4(m, dm);
      pose::set_local_pose(
        sprite_system::get_pose(application::world(world).sprite_system, sprite), dm);
    }

    u64 unit(u64 world, u64 sprite)
    {
      World &w = application::world(world);
      const UnitRef ur ={ 0 };
      return hash::get(w.sprite_unit, sprite, ur).unit;
    }

    void box(u64 world, u64 sprite, Box &box)
    {
      const Sprite *s = idlut::lookup(application::world(world).sprite_system.sprites, sprite);
      AABB *aabb = idlut::lookup(application::world(world).culling_system, s->aabb);
      box.min = aabb->data[0];
      box.max = aabb->data[1];
    }

    bool is_visible(u64 world, u64 sprite)
    {
      const Sprite *s = idlut::lookup(application::world(world).sprite_system.sprites, sprite);
      return idlut::lookup(application::world(world).culling_system, s->aabb)->visible;
    }

    void get_color(u64 world, u64 sprite, Color &color)
    {
      sprite_system::get_color(application::world(world).sprite_system, sprite, color);
    }

    void set_color(u64 world, u64 sprite, const Color &color)
    {
      sprite_system::set_color(application::world(world).sprite_system, sprite, color);
    }
  }
}