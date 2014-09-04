#include <runtime/array.h>
#include <runtime/memory.h>
#include "sample.h"

#define PI 3.14159265359f

namespace app
{
  namespace
  {
    using namespace pge;
    glm::vec3 pcam(0, 0, 1000);
    u64 sprites[8];
    u64 viewport;

    void spawn_units(glm::vec3 p, f32 r)
    {
      glm::vec3 points[8];

      points[0] = glm::vec3((p.x + r*sin((1.0f / 8)*PI)), p.y, (p.z + r*cos((1.0f / 8)*PI)));
      points[1] = glm::vec3((p.x + r*sin((3.0f / 8)*PI)), p.y, (p.z + r*cos((3.0f / 8)*PI)));
      points[2] = glm::vec3((p.x + r*sin((5.0f / 8)*PI)), p.y, (p.z + r*cos((5.0f / 8)*PI)));
      points[3] = glm::vec3((p.x + r*sin((7.0f / 8)*PI)), p.y, (p.z + r*cos((7.0f / 8)*PI)));
      points[4] = glm::vec3((p.x + r*sin((9.0f / 8)*PI)), p.y, (p.z + r*cos((9.0f / 8)*PI)));
      points[5] = glm::vec3((p.x + r*sin((11.0f / 8)*PI)), p.y, (p.z + r*cos((11.0f / 8)*PI)));
      points[6] = glm::vec3((p.x + r*sin((13.0f / 8)*PI)), p.y, (p.z + r*cos((13.0f / 8)*PI)));
      points[7] = glm::vec3((p.x + r*sin((15.0f / 8)*PI)), p.y, (p.z + r*cos((15.0f / 8)*PI)));

      for (i32 i = 0; i < 8; i++)
        sprites[i] = world::spawn_sprite(global_game_world, "sprites/ground", points[i], glm::quat(1, 0, 0, 0));
    }
  }


  namespace sample_culling
  {
    Camera scam;

    char *description = "3D navigation enable (pad, keyboard+mouse)\nz,q,s and d to orientate frustrum";

    void init()
    {
      global_sample_desciption = description;

      //level = world::load_level(global_game_world, "levels/culling_test/level", glm::vec3(0, 0, 0), glm::quat(1, 0, 0, 0));
      spawn_units(glm::vec3(0, 0, 0), 1024);

      camera_free::initialize(pcam);
      global_game_camera.set_near_range(1.0f);

      scam.init(global_game_world, CAMERA_MODE_FREE, (f32)global_screen.width, (f32)global_screen.height);
      scam.set_near_range(0.1f);
      scam.set_far_range(1000.0f);
      scam.set_vertical_fov(45.0f);

      viewport = application::create_viewport(0, 0, 320, 240);
    }


    inline void handle_pad(f64 dt)
    {
      glm::vec3 a = scam.get_euler_angles();
      f32 speed = scam.get_rotation_speed();

      if (pad::button(global_pad_index, PAD_KEY_14)) // left
        a.y += speed * (f32)dt;

      if (pad::button(global_pad_index, PAD_KEY_11)) // up
        a.x += speed * (f32)dt;

      if (pad::button(global_pad_index, PAD_KEY_12)) // right
        a.y -= speed * (f32)dt;

      if (pad::button(global_pad_index, PAD_KEY_13)) // down
        a.x -= speed * (f32)dt;

      scam.set_euler_angles(a);
      scam.update();

    }

    inline void handle_keyboard(f64 dt)
    {
      glm::vec3 a = scam.get_euler_angles();
      f32 speed = scam.get_rotation_speed();

      if (keyboard::button(KEYBOARD_KEY_Q)) // left
        a.y += speed * (f32)dt;

      if (keyboard::button(KEYBOARD_KEY_Z)) // up
        a.x += speed * (f32)dt;

      if (keyboard::button(KEYBOARD_KEY_D)) // right
        a.y -= speed * (f32)dt;

      if (keyboard::button(KEYBOARD_KEY_S)) // down
        a.x -= speed * (f32)dt;

      scam.set_euler_angles(a);
      scam.update();
    }

    void update(f64 dt)
    {
      camera_free::update(dt);


      if (pad::active(global_pad_index))
        handle_pad(dt);

      handle_keyboard(dt);
    }

    void synchronize()
    {

    }

    void render()
    {
      glm::vec3 p;
      camera::get_local_translation(global_game_world, scam.get_id(), p);
      application::render_circle(p, 20, Color(255, 255, 255, 255), global_game_world, global_game_camera.get_id(), global_viewport);
      application::render_frustum(scam.get_id(), global_game_world, global_game_camera.get_id(), global_viewport);
      application::render_world(global_game_world, scam.get_id(), viewport);

      for (i32 i = 0; i < 8; i++) {
        if (sprite::is_visible(global_game_world, sprites[i])) {
          sprite::get_world_position(global_game_world, sprites[i], p);
          application::render_circle(p, 20, Color(0, 255, 0, 255), global_game_world, global_game_camera.get_id(), global_viewport);
        }
      }
    }

    void shutdown()
    {
      for (u32 i = 0; i < 8; i++)
        world::despawn_sprite(global_game_world, sprites[i]);
      camera_free::reset_ortho();
      application::destroy_viewport(viewport);
    }
  }
}