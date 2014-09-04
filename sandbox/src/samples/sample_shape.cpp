#include <runtime/array.h>
#include <runtime/trace.h>
#include "sample.h"


#include <glm/glm.hpp>


namespace app
{
  using namespace pge;

  namespace sample_shape
  {
    Array<u64> *actors = NULL;

    void init()
    {
      Allocator &a = memory_globals::default_allocator();
      actors = MAKE_NEW(a, Array<u64>, a);

      //global_debug_physic = true;

      glm::quat q1 = glm::angleAxis(glm::radians(45.f), glm::vec3(0, 0, 1));

      f32 w4 = global_screen.width*.25f;
      f32 h4 = global_screen.height*.25f;

      f32 w8 = global_screen.width*.125f;
      f32 h8 = global_screen.height*.125f;

      glm::vec3 top_left(-w4, h4, 0);
      glm::vec3 top_right(w4, h4, 0);

      glm::vec3 bottom_left(-w4, -h4, 0);
      glm::vec3 bottom_right(w4, -h4, 0);
      
      //*
      array::push_back(*actors, world::spawn_actor_box(global_game_world, w8, h8,
        true, true, false, false,
        "default", "default_static",
        top_left, q1));

      array::push_back(*actors, world::spawn_actor_circle(global_game_world, h8,
        false, false, false, false,
        "default", "default_static",
        top_right, q1));

      glm::vec2 chain_vertices[] ={
        glm::vec2(-100, -50),
        glm::vec2(-50, 50),
        glm::vec2(50, -50),
        glm::vec2(100, 50)
      };
      array::push_back(*actors, world::spawn_actor_chain(global_game_world, chain_vertices, 4u,
        false, true, false, false,
        "default", "default_static",
        bottom_left, q1));
      //*/

      glm::vec2 polygon_vertices[] ={
        glm::vec2(0, 150),
        glm::vec2(150, -150),
        glm::vec2(-150, -150),
      };
      array::push_back(*actors, world::spawn_actor_polygon(global_game_world, polygon_vertices, 3u,
        false, false, false, false,
        "default", "default_static",
        bottom_right, q1));
    }

    void shutdown()
    {
      u64 *actor, *end = array::end(*actors);
      for (actor = array::begin(*actors); actor < end; actor++)
        world::despawn_actor(global_game_world, *actor);

      MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, actors);
    }
  }
}