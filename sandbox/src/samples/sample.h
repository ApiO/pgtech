#pragma once

#include <stdio.h>
#include <engine/pge.h>
#include "../global.h"

namespace app
{
  inline int randomize(int min, int max)
  {
    return (rand() % (max - min + 1)) + min;
  }

  namespace sample_screen_to_world
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_default
  {
    void init();
    void update(pge::f64 dt);
    void synchronize();
    void shutdown();
  }

  namespace sample_shape
  {
    void init();
    void shutdown();
  }

  namespace sample_level
  {
    void init();
    void shutdown();
  }

  namespace sample_actor_raycast
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_actor_touch
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_unit
  {
    void init();
    void shutdown();
  }

  namespace sample_super_spineboy
  {
    void init();
    void update(pge::f64 dt);
    void synchronize();
    void shutdown();
  }

  namespace sample_stress_my_balls
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_physics
  {
    void init();
    void shutdown();
  }

  namespace sample_mr_patate
  {
    void init();
    void shutdown();
  }

  namespace sample_pads
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_mouse
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_keyboard
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_audio
  {
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_font
  {
    void init();
    void shutdown();
  }

  namespace sample_geometry
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_camera
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_mover
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_sprite
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_culling
  {
    void init();
    void update(pge::f64 dt);
    void render();
    void shutdown();
  }

  namespace sample_blend
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace sample_selection
  {
    void init();
    void update(pge::f64 dt);
    void render();
    void shutdown();
  }

  namespace sample_particles
  {
    void init();
    void update(pge::f64 dt);
    void shutdown();
  }

  namespace camera_free
  {
    using namespace pge;

    static glm::vec3 default_translation(0.f);
    static glm::vec3 default_rotation(0.f);
    static glm::vec3 last_rotation(0.f);
    static glm::vec3 last_position(0.f);

    static f32 horizontal_angle = 3.14f;
    static f32 vertical_angle = 0.0f;
    static glm::vec2 previous_mp;

    const f32 PAD_SENSITIVITY = .7f;
    const f32 MOUSE_SPEED = .1f;

    inline void initialize(glm::vec3 &position)
    {
      window::display_cursor(false);
      global_game_camera.set_mode(CAMERA_MODE_FREE, .1f, 10000.f);

      default_translation = position;
      last_position = position;

      horizontal_angle = 3.14f;
      vertical_angle = 0.0f;

      previous_mp = glm::vec2(global_screen.w2, global_screen.h2);
      mouse::set_position(previous_mp);

      global_game_camera.set_position(default_translation);
    }

    inline void handle_pad(f64 dt)
    {
      if (pad::button(global_pad_index, 0))
      {
        global_game_camera.set_position(default_translation);
        last_position = default_translation;

        global_game_camera.set_euler_angles(default_rotation);
        last_rotation = default_rotation;

        return;
      }

      glm::vec3 position = global_game_camera.get_position();
      glm::vec3 euler_angles = global_game_camera.get_euler_angles();

      // TRANSLATION: left stick
      if (pad::pressed(global_pad_index, 8)){
        global_game_camera.set_position(default_translation);
      }
      else {
        // X MOVES
        f32 left_stick_horiz = pad::axes(global_pad_index, 0);
        if (left_stick_horiz > PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_RIGHT, dt);
        }
        else if (left_stick_horiz < -PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_LEFT, dt);
        }

        // Y MOVES
        f32 left_stick_vert = pad::axes(global_pad_index, 1);
        if (left_stick_vert > PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_BACKWARD, dt);
        }
        else if (left_stick_vert < -PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_FORWARD, dt);
        }

        // Z Moves - trigger top
        if (pad::button(global_pad_index, 4)){
          global_game_camera.move(CAMERA_MOVE_DOWN, dt);
        }
        else if (pad::button(global_pad_index, 5)){
          global_game_camera.move(CAMERA_MOVE_UP, dt);
        }
      }

      // ROTATION: right stick
      if (pad::pressed(global_pad_index, 9)){
        global_game_camera.set_euler_angles(default_rotation);
      }
      else {
        // X
        bool x_change, y_change, z_change;
        x_change = y_change = z_change = false;

        f32 right_stick_horiz = pad::axes(global_pad_index, 3);
        if (right_stick_horiz > PAD_SENSITIVITY){
          euler_angles.x -= f32(dt * global_game_camera.get_rotation_speed());
          x_change = true;
        }
        else if (right_stick_horiz < -PAD_SENSITIVITY){
          euler_angles.x += f32(dt * global_game_camera.get_rotation_speed());
          x_change = true;
        }
        if (x_change)
          if (euler_angles.x <= -360.f){
          euler_angles.x += 360.f;
          }
          else if (euler_angles.x >= 360.f){
            euler_angles.x -= 360.f;
          }

          // Y
          f32 right_stick_vert = pad::axes(global_pad_index, 4);
          if (right_stick_vert > PAD_SENSITIVITY){
            euler_angles.y -= f32(dt * global_game_camera.get_rotation_speed());
            y_change = true;
          }
          else if (right_stick_vert < -PAD_SENSITIVITY){
            euler_angles.y += f32(dt * global_game_camera.get_rotation_speed());
            y_change = true;
          }
          if (y_change)
            if (euler_angles.y <= -360.f){
            euler_angles.y += 360.f;
            }
            else if (euler_angles.y >= 360.f){
              euler_angles.y -= 360.f;
            }

            // Z
            f32 triggers = pad::axes(global_pad_index, 2);
            if (triggers > PAD_SENSITIVITY){
              euler_angles.z -= f32(dt * global_game_camera.get_rotation_speed());
              z_change = true;
            }
            else if (triggers < -PAD_SENSITIVITY){
              euler_angles.z += f32(dt * global_game_camera.get_rotation_speed());
              z_change = true;
            }
            if (z_change)
              if (euler_angles.z <= -360.f){
              euler_angles.z += 360.f;
              }
              else if (euler_angles.z >= 360.f){
                euler_angles.z -= 360.f;
              }
      }

      if (last_rotation != euler_angles)
      {
        global_game_camera.set_euler_angles(euler_angles);
        last_rotation = euler_angles;
      }

      if (last_position != position)
      {
        global_game_camera.set_position(position);
        last_position = position;
      }
    }

    inline void handle_mouse_and_kb(f64 dt)
    {
      // handle keyboard
      if (keyboard::pressed(KEYBOARD_KEY_DELETE))
      {
        global_game_camera.set_position(default_translation);
        last_position = default_translation;

        global_game_camera.set_euler_angles(default_rotation);
        last_rotation = default_rotation;

        return;
      }
      if (keyboard::button(KEYBOARD_KEY_LEFT)){
        global_game_camera.move(CAMERA_MOVE_LEFT, dt);
      }
      else if (keyboard::button(KEYBOARD_KEY_RIGHT)){
        global_game_camera.move(CAMERA_MOVE_RIGHT, dt);
      }

      if (keyboard::button(KEYBOARD_KEY_UP)){
        global_game_camera.move(CAMERA_MOVE_FORWARD, dt);
      }
      else if (keyboard::button(KEYBOARD_KEY_DOWN)){
        global_game_camera.move(CAMERA_MOVE_BACKWARD, dt);
      }

      if (keyboard::button(KEYBOARD_KEY_RIGHT_CONTROL)){
        global_game_camera.move(CAMERA_MOVE_DOWN, dt);
      }
      else if (keyboard::button(KEYBOARD_KEY_RIGHT_SHIFT)){
        global_game_camera.move(CAMERA_MOVE_UP, dt);
      }

      // handle mouse
      glm::vec2 mp = mouse::get_position();

      if (previous_mp == mp) return;

      previous_mp = glm::vec2(global_screen.w2, global_screen.h2);
      mouse::set_position(previous_mp);
      previous_mp = mp;

      horizontal_angle += MOUSE_SPEED * (global_screen.w2 - mp.x);
      vertical_angle += MOUSE_SPEED * (global_screen.h2 - mp.y);

      glm::vec3 direction(vertical_angle, horizontal_angle, 0);

      global_game_camera.set_euler_angles(direction);
    }

    inline void update(f64 dt)
    {

      if (global_pad_index > -1 && pad::active(global_pad_index))
        handle_pad(dt);

      handle_mouse_and_kb(dt);

    }

    inline void reset_ortho()
    {
      window::display_cursor(true);
      global_game_camera.set_mode(CAMERA_MODE_OTRHO, -1.f, 1.f);
      glm::vec3 zero(0.f);

      global_game_camera.set_position(zero);
      global_game_camera.set_euler_angles(zero);

      last_rotation = last_position = zero;
    }
  }
}