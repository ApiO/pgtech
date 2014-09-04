#include <runtime/array.h>
#include "sample.h"

namespace app
{
  using namespace pge;

  const glm::vec3 TRANSLATION(0.f);
  const glm::quat ROTATION(1.f, 0.f, 0.f, 0.f);
  
  namespace sample_geometry
  {
    Color _color(0, 255, 255, 255);
    Color _color0(255, 255, 255, 255);
    Color _color1(255, 0, 0, 255);
    Color _color2(0, 255, 0, 255);
    Color _color3(0, 0, 255, 255);

    Color tgray(255, 255, 255, 140);
    Color turquoise(0, 255, 255, 255);
    Color epic(163, 53, 238, 255);
    Color rare(237, 255, 64, 255);
    Color legendary(176, 91, 42, 255);

    Array<u64> *geometries = NULL;

    char *description = "3D navigation enable (pad, keyboard+mouse)";


    void init()
    {
      global_sample_desciption = description;

      Allocator &a = memory_globals::default_allocator();
      geometries = MAKE_NEW(a, Array<u64>, a);

      {
        f32 spacing = 1000.f;
        f32 axe_len = 10000.f;
        glm::vec3 origin(0.f);
        glm::vec3 a, b;

        for (f32 i = -axe_len; i <= axe_len; i+=spacing)
        {
          if (i == 0.f) continue;

          a = glm::vec3(-axe_len, 0, 0);
          b = glm::vec3(axe_len, 0, 0);
          array::push_back(*geometries, world::spawn_line(global_game_world, a, b, tgray, glm::vec3(0, 0, i), ROTATION));

          a = glm::vec3(0, 0, -axe_len);
          b = glm::vec3(0, 0, axe_len);
          array::push_back(*geometries, world::spawn_line(global_game_world, a, b, tgray, glm::vec3(i, 0, 0), ROTATION));
        }

        // AXE X
        a = glm::vec3(-axe_len, 0, 0);
        array::push_back(*geometries, world::spawn_line(global_game_world, a, origin, tgray, TRANSLATION, ROTATION));
        b = glm::vec3(axe_len, 0, 0.f);
        array::push_back(*geometries, world::spawn_line(global_game_world, origin, b, _color2, TRANSLATION, ROTATION));

        // AXE Y
        a = glm::vec3(0, -axe_len, 0);
        array::push_back(*geometries, world::spawn_line(global_game_world, a, origin, tgray, TRANSLATION, ROTATION));
        b = glm::vec3(0, axe_len, 0);
        array::push_back(*geometries, world::spawn_line(global_game_world, origin, b, _color1, TRANSLATION, ROTATION));

        // AXE Y
        a = glm::vec3(0, 0, -axe_len);
        array::push_back(*geometries, world::spawn_line(global_game_world, a, origin, tgray, TRANSLATION, ROTATION));
        b = glm::vec3(0, 0, axe_len);
        array::push_back(*geometries, world::spawn_line(global_game_world, origin, b, _color3, TRANSLATION, ROTATION));
      }
      //*/


      // STAR

      glm::vec3 points[] ={
        glm::vec3(0.f, 200.f, 0.f),
        glm::vec3(-50.f, 50.f, 200.f),
        glm::vec3(-200.f, 0.f, 0.f),
        glm::vec3(-50.f, -50.f, 200.f),
        glm::vec3(0.f, -200.f, 0.f),
        glm::vec3(50.f, -50.f, 200.f),
        glm::vec3(200.f, 0.f, 0.f),
        glm::vec3(50.f, 50.f, 200.f)
      };
      array::push_back(*geometries, world::spawn_polygon(global_game_world, &points[0], 8, _color1, glm::vec3(600.f, 600.f, 0.f), glm::quat(glm::radians(glm::vec3(0.f, 45.f, 45.f)))));

      //*
      glm::vec3 center;

      center = glm::vec3(0.f, 200.f, 300.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color2, TRANSLATION, ROTATION));

      center = glm::vec3(0.f, -200.f, 300.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color2, TRANSLATION, ROTATION));


      center = glm::vec3(200.f, 0.f, 300.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color2, TRANSLATION, ROTATION));

      center = glm::vec3(-200.f, 0.f, 300.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color2, TRANSLATION, ROTATION));


      center = glm::vec3(300.f, -300.f, 500.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color3, TRANSLATION, ROTATION, true));

      center = glm::vec3(-300.f, -300.f, 500.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color3, TRANSLATION, ROTATION, true));

      center = glm::vec3(-300.f, 300.f, 500.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color3, TRANSLATION, ROTATION, true));

      center = glm::vec3(300.f, 300.f, 500.f);
      array::push_back(*geometries, world::spawn_circle(global_game_world, center, 100.f, _color3, TRANSLATION, ROTATION, true));

      Color colors[] =
      {
        Color(255, 0, 0, 255),
        Color(225, 0, 220, 255),
        Color(255, 186, 17, 255),
        Color(18, 127, 72, 255),
        Color(65, 142, 0, 255),
        Color(0, 255, 0, 255),
        Color(0, 0, 255, 255)
      };

      for (i32 i = -300; i <= 300; i+=100)
      {
        glm::vec3 c0(0.f, 0.f, i);
        array::push_back(*geometries, world::spawn_circle(global_game_world, c0, 100.f, colors[u32((i / 100) + 3)], TRANSLATION, ROTATION, true));

        glm::vec3 points[] ={

          glm::vec3(170.f, 20.f, i),
          glm::vec3(170.f, -20.f, i),
          glm::vec3(130.f, -20.f, i),
          glm::vec3(130.f, 20.f, i),
          glm::vec3(0.f, 170.f, i),
          glm::vec3(20.f, 130.f, i),
          glm::vec3(-20.f, 130.f, i)
        };
        array::push_back(*geometries, world::spawn_polygon(global_game_world, points, 4, _color, TRANSLATION, ROTATION));
        array::push_back(*geometries, world::spawn_polygon(global_game_world, &points[4], 3, _color, TRANSLATION, ROTATION));
      }

      glm::vec3 psition(0, 0, 1200);
      camera_free::initialize(psition);
    }

    void update(f64 dt)
    {
      camera_free::update(dt);
    }

    void shutdown()
    {
      u64 *geometry, *end = array::end(*geometries);
      for (geometry = array::begin(*geometries); geometry < end; geometry++)
        world::despawn_geometry(global_game_world, *geometry);

      MAKE_DELETE(memory_globals::default_allocator(), Array<u64>, geometries);

      camera_free::reset_ortho();
    }
  }
}