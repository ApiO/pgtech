#include <runtime/array.h>
#include "sample.h"

namespace app
{
  namespace
  {
    bool intersect_plane(const glm::vec3 &n, const glm::vec3 &p0, const glm::vec3& l0, glm::vec3 &l, float &d)
    {
      glm::vec3 p0l0;
      // assuming vectors are all normalized
      float denom = glm::dot(n, l);
      if (denom > 1e-6) {
        p0l0 = p0 - l0;
        d = glm::dot(p0l0, n) / denom;
        return (d >= 0);
      }
      return false;
    }
  }

  namespace sample_screen_to_world
  {
    using namespace pge;

    Array<u64> *sprites = NULL;
    u64 mouse_cursor;
    f32 world_z;

    void init()
    {
      Allocator &a = memory_globals::default_allocator();
      glm::vec3 p(0, 0, 900);
      camera_free::initialize(p);
      world_z = 0;
      mouse_cursor = world::spawn_circle(global_game_world, glm::vec3(0, 0, 0), 5, Color(255, 255, 255, 255), glm::vec3(0, 0, 0), glm::quat(1, 0, 0, 0));
      sprites = MAKE_NEW(a, Array<u64>, a);

      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/dragon/sprites/R_wing", glm::vec3(-600, 0, 0), glm::quat(1, 0, 0, 0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/dragon/sprites/L_wing", glm::vec3(-100, 0, 0), glm::quat(1, 0, 0, 0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/spineboy/sprites/eyes", glm::vec3(250, 0, 0), glm::quat(1, 0, 0, 0)));
      array::push_back(*sprites, world::spawn_sprite(global_game_world, "units/spineboy/sprites/head", glm::vec3(350, 0, 0), glm::quat(1, 0, 0, 0)));
    }

    void update(f64 dt)
    {
      camera_free::update(dt);
      glm::vec2 mouse_pos = mouse::get_position();

      glm::vec3 pn, pf, dir;
      f32 l;

      world_z += 5 * mouse::wheel_scroll();
      world_z = glm::clamp(world_z, 99.0f, 899.0f);

      camera::screen_to_world(global_game_world, global_game_camera.get_id(), glm::vec3(mouse_pos.x, mouse_pos.y, 0), pn);
      camera::screen_to_world(global_game_world, global_game_camera.get_id(), glm::vec3(mouse_pos.x, mouse_pos.y, 1), pf);

      dir = glm::normalize(pf - pn);

      if (intersect_plane(glm::vec3(0, 0, -1), glm::vec3(0, 0, world_z), pn, dir, l))
        geometry::set_local_position(global_game_world, mouse_cursor, pn + dir*l);
      else
        printf("%f : %f, %f\n", world_z, pn.z, pf.z);


      if (mouse::pressed(MOUSE_KEY_1)) {
        Box b;
        i32 num_selected = 0;
        for (u32 i = 0; i < array::size(*sprites); i++) {
          sprite::box(global_game_world, (*sprites)[i], b);
          if (math::ray_box_intersection(pn, dir, b)) {
            printf("selected : %llu\n", (*sprites)[i]);
            num_selected++;
          }
        }

        if (!num_selected)
          printf("no sprite selected\n");
      }
    }


    void shutdown()
    {
      Allocator &a = memory_globals::default_allocator();

      for (u32 i = 0; i < array::size(*sprites); i++)
        world::despawn_sprite(global_game_world, (*sprites)[i]);

      MAKE_DELETE(a, Array<u64>, sprites);
      world::despawn_geometry(global_game_world, mouse_cursor);
      camera_free::reset_ortho();
    }
  }
}