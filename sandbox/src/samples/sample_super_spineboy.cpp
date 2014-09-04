#include "sample.h"

#include <engine/matrix.h>

namespace
{
  using namespace pge;
  using namespace app;

  u64 boy = NULL;
  u64 wall = NULL;
  //u64 pseudo_text = NULL;
  u64 sword_text  = NULL;

  bool first_loop = true;
  bool flip_x = false;

  glm::vec3 bulle_offset(-60, 400, 0),
    sword_offset(-52, 300, 0);

  //u64 line, circle, arrow;

  bool look_ritgh;
}

namespace app
{
  using namespace pge;

  namespace sample_super_spineboy
  {
    void init()
    {
      glm::vec3 position(0.f, -104.f, 0.f);
      //glm::vec3 position(0.f, -global_screen.h2 + 104.f, 0.f);
      glm::quat rot = glm::angleAxis(0.f, glm::vec3(0.f, 0.f, 1.f));

      boy = world::spawn_unit(global_game_world, "units/spineboy/spineboy", position, rot);
      unit::play_animation(global_game_world, boy, "walk", 0, 0, true, 1.f);
      look_ritgh = true;

      wall = world::spawn_unit(global_game_world, "units/ground", IDENTITY_TRANSLATION, IDENTITY_ROTATION);

      /*
      {
      glm::mat4 m(1.f);
      m = glm::scale(m, glm::vec3(.5f, .5f, 1.f));
      unit::set_local_pose(global_game_world, boy, 0, m);
      }
      //*/
      /*
      f32 a = 25.f, len = 8;
      glm::vec3 poly[]={
        glm::vec3(-a, 0.f, 0.f),
        glm::vec3(-a, len * a, 0.f),
        glm::vec3(-2 * a, len * a, 0.f),
        glm::vec3(0, (len+2) * a, 0.f),
        glm::vec3(2 * a, len * a, 0.f),
        glm::vec3(a, len * a, 0.f),
        glm::vec3(a, a, 0.f)
      };
      Color poly_color(255, 0, 0, 255);
      arrow = world::spawn_polygon(global_game_world, poly, 7, poly_color, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      */

      //*
      {
        world::link_unit(global_game_world, wall, boy, 17);

        glm::mat4 m(1.f);
        m = m * glm::toMat4(glm::angleAxis(glm::radians(45.f), glm::vec3(0, 0, 1)));
        m = glm::scale(m, glm::vec3(.25f, 1.f, 1.f));
        unit::set_local_pose(global_game_world, wall, 0, m);
      }
      //*/

      // ajout du nom du perso sur la tete
      //pseudo_text = world::spawn_text(global_game_world, global_font_name, "Je suis une patate\n__________\n     \\", TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);

      //*
      sword_text  = world::spawn_text(global_game_world, global_font_name,
                                      "   ___\n  |   |\n  |   |\n  |   |\n  |   |\n  |   |\n  |   |\n  |   |\n  |   |\n  |   |\n  < O >\n    |\n\n    |",
                                      TEXT_ALIGN_LEFT, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
      //*/

      /*
      line   = world::spawn_line(global_game_world, glm::vec3(-800, 0, 0), glm::vec3(800, 0, 0), Color(12, 142, 12, 255), position, IDENTITY_ROTATION);
      circle = world::spawn_circle(global_game_world, glm::vec3(global_screen.width*.25f, global_screen.height*.25f, 0), 100, Color(255, 255, 0, 255), IDENTITY_TRANSLATION, IDENTITY_ROTATION, true);
      */

      synchronize();
    }

    void update(f64 dt)
    {
      (void)dt;
      return;
      /*
      glm::vec3 boy_position;
      unit::get_local_position(global_game_world, boy, 0, boy_position);

      if (keyboard::button(KEYBOARD_KEY_LEFT_CONTROL)) {
        unit::play_animation(global_game_world, boy, "jump");
      }

      if (keyboard::button(KEYBOARD_KEY_Z)){
        boy_position.y += f32(global_game_camera.get_move_speed()*dt);
        global_game_camera.move(CAMERA_MOVE_UP, dt);
        unit::set_local_position(global_game_world, boy, 0, boy_position);
      }
      else if (keyboard::button(KEYBOARD_KEY_S)){
        boy_position.y -= f32(global_game_camera.get_move_speed()*dt);
        global_game_camera.move(CAMERA_MOVE_DOWN, dt);
        unit::set_local_position(global_game_world, boy, 0, boy_position);
      }

      if (keyboard::button(KEYBOARD_KEY_D)){
        boy_position.x += f32(global_game_camera.get_move_speed()*dt);
        global_game_camera.move(CAMERA_MOVE_RIGHT, dt);
        unit::set_local_position(global_game_world, boy, 0, boy_position);
        if (!look_ritgh)
        {
          unit::set_local_rotation(global_game_world, boy, 0, IDENTITY_ROTATION);
          look_ritgh = true;
        }
      }
      else if (keyboard::button(KEYBOARD_KEY_Q)){
        boy_position.x -= f32(global_game_camera.get_move_speed()*dt);
        global_game_camera.move(CAMERA_MOVE_LEFT, dt);
        unit::set_local_position(global_game_world, boy, 0, boy_position);
        if (look_ritgh)
        {
          unit::set_local_rotation(global_game_world, boy, 0, glm::angleAxis(glm::radians(180.f), glm::vec3(0, 1, 0)));
          look_ritgh = false;
        }
      }

      if (keyboard::button(KEYBOARD_KEY_SPACE))
      {
        glm::vec3 r = global_game_camera.get_euler_angles();
        r.z += f32(dt * global_game_camera.get_rotation_speed());
        global_game_camera.set_euler_angles(r);
      }
      //*/
    }

    void synchronize()
    {
      // bulle
      {
        glm::vec3 boy_position;
        unit::get_local_position(global_game_world, boy, 0, boy_position);
        //text::set_local_position(global_game_world, pseudo_text, boy_position + bulle_offset);
      }


      // sword
      //*
      {
        glm::mat4 hand_world_pose;
        unit::get_world_pose(global_game_world, boy, 16, hand_world_pose);

        glm::mat4 sword_world_pose = hand_world_pose * glm::translate(IDENTITY_MAT4, sword_offset);

        text::set_local_pose(global_game_world, sword_text, sword_world_pose);
      }
      //*/


      // arraow
      /*
      {
        glm::mat4 hand_world_pose;
        unit::get_world_pose(global_game_world, boy, 16, hand_world_pose);

        glm::mat4 arrow_world_pose = hand_world_pose * glm::translate(IDENTITY_MAT4, IDENTITY_TRANSLATION);

        geometry::set_local_pose(global_game_world, arrow, arrow_world_pose);
      }
      //*/
    }

    void shutdown()
    {
      world::unlink_unit(global_game_world, wall);

      world::despawn_unit(global_game_world, wall);
      world::despawn_unit(global_game_world, boy);

      //world::despawn_text(global_game_world, pseudo_text);
      world::despawn_text(global_game_world, sword_text);

      //world::despawn_geometry(global_game_world, line);
      //world::despawn_geometry(global_game_world, circle);
      //world::despawn_geometry(global_game_world, arrow);

      camera_free::reset_ortho();
    }

  }
}