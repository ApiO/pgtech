#include <runtime/array.h>
#include <runtime/trace.h>
#include "sample.h"

namespace
{
  using namespace pge;
  using namespace app;

  u64 raycast;
  glm::vec3 raycast_from(0);
  glm::vec3 raycast_to;
  f32 raycast_angle = 0.f;

  f32 RAY_ROTATION_SPEED = 30;

  void raycast_callback(const Array<ContactPoint> &hits)
  {
    const ContactPoint *cp, *end = array::end(hits);
    for (cp = array::begin(hits); cp < end; cp++)
    {
      application::render_line(cp->position, (cp->position + (cp->normal)),
                               Color(0, 255, 0, 255),
                               global_game_world, global_game_camera.get_id(), global_viewport);
      application::render_circle(cp->position, 4,
                                 Color(255, 0, 0, 255),
                                 global_game_world, global_game_camera.get_id(), global_viewport);
    }
  }
}


namespace app
{
  using namespace pge;

  namespace sample_actor_raycast
  {
    u64 box, circle, chain, polygon;

    void init()
    {
      global_debug_physic = true;
      
      raycast_to = glm::vec3(global_screen.width*0.5f, 0, 0);

      raycast = physics::create_raycast(global_game_world, raycast_callback, "default", false);


      glm::quat q = glm::angleAxis(glm::radians(45.f), glm::vec3(0, 0, 1));

      f32 w = global_screen.width*.25f;
      f32 h = global_screen.height*.25f;

      glm::vec3 top_left(-w, h, 0);
      glm::vec3 top_right(w, h, 0);

      glm::vec3 bottom_left(-w, -h, 0);
      glm::vec3 bottom_right(w, -h, 0);

      box = world::spawn_actor_box(global_game_world, 300, 100,
                                  true, true, false, true,
                                  "default", "default_static",
                                  top_left, q);

      circle = world::spawn_actor_circle(global_game_world, 200,
                                         true, true, false, true,
                                         "default", "default_static",
                                         top_right, q);

      glm::vec2 chain_vertices[] ={
        glm::vec2(-100, -50),
        glm::vec2(-50, 50),
        glm::vec2(50, -50),
        glm::vec2(100, 50),
        glm::vec2(200, 50),
        glm::vec2(400, -50)
      };
      chain = world::spawn_actor_chain(global_game_world, chain_vertices, 6u,
                                           true, true, false, true,
                                           "default", "default_static",
                                           bottom_left, q);

      glm::vec2 verts[] ={
        glm::vec2(0, 150),
        glm::vec2(150, -150),
        glm::vec2(-150, -150),
      };
      polygon = world::spawn_actor_polygon(global_game_world, verts, 3u,
                                           true, true, false, true,
                                           "default", "default_static",
                                           bottom_right, q);
    }

    void update(f64 dt)
    {
      {
        f32 radians = glm::radians((f32)(RAY_ROTATION_SPEED * dt));
        f32 cs = cos(radians);
        f32 sn = sin(radians);
        raycast_to = glm::vec3(raycast_to.x * cs - raycast_to.y * sn,
                               raycast_to.x * sn + raycast_to.y * cs, 0);
      }

      physics::cast_raycast(global_game_world, raycast, raycast_from, raycast_to);

      application::render_line(raycast_from, raycast_to,
                               Color(0, 255, 255, 142),
                               global_game_world, global_game_camera.get_id(), global_viewport);
    }

    void shutdown()
    {
      physics::destroy_raycast(global_game_world, raycast);

      world::despawn_actor(global_game_world, box);
      world::despawn_actor(global_game_world, circle);
      world::despawn_actor(global_game_world, chain);
      world::despawn_actor(global_game_world, polygon);
    }
  }
}