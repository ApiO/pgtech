#include <stdio.h>

#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_camera
  {
    void set_mode_infos();
    void print_camera();
  }

  namespace sample_camera
  {
    const char *PROJECTION_NAMES[] ={ "FREE", "ORTHOGONALE" };
    const pge::f32 PAD_SENSITIVITY = .7f;

    u64 projection_text;
    u64 cam_info_labels;
    u64 cam_info_values;
    u64 cam_mode_infos;

    glm::vec3 last_rotation(0.f);
    glm::vec3 last_position(0.f);

    void set_mode_infos()
    {
      CameraMode mode = global_game_camera.get_mode();
      text::set_string(global_gui_world, projection_text, PROJECTION_NAMES[mode]);

      //cam_mode_infos
      char buf[512];
      glm::vec3 up      = global_game_camera.get_up();
      glm::vec3 forward = global_game_camera.get_forward();
      f32 z_near        = global_game_camera.get_near_range();
      f32 z_far         = global_game_camera.get_far_range();

      if (mode == CAMERA_MODE_OTRHO)
        sprintf(buf, "left:   %.1f\nright:   %.1f\nbottom: %.1f\ntop:     %.1f\nz_near:  %.1f\nz_far:   %.1f\nup:     (%.0f,%.0f,%.0f)\nforward:(%.0f,%.0f,%.0f)",
        f32(-global_screen.w2), f32(global_screen.w2), f32(-global_screen.h2), f32(global_screen.h2),
        z_near, z_far,
        up.x, up.y, up.z,
        forward.x, forward.y, forward.z);
      else
        sprintf(buf, "fov:     %.1f\nz_near:  %.1f\nz_far:   %.1f\nup:     (%.0f,%.0f,%.0f)\nforward:(%.0f,%.0f,%.0f)",
        global_game_camera.get_vertical_fov(),
        z_near, z_far,
        up.x, up.y, up.z,
        forward.x, forward.y, forward.z);

      text::set_string(global_gui_world, cam_mode_infos, buf);
    }

    void print_camera()
    {
      char buf[512];
      sprintf(buf, "%.1f\n%.1f\n%.1f\n\n\n%.1f\n%.1f\n%.1f", last_position.x, last_position.y, last_position.z
              , last_rotation.x, last_rotation.y, last_rotation.z);
      text::set_string(global_gui_world, cam_info_values, buf);
    }


    void init()
    {

      projection_text = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 6, global_screen.h2 - 6, 0.f), IDENTITY_ROTATION);

      char *cam_info  = "Position:\n x = \n y =\n z =\n\nRotation:\n x =\n y =\n z =";
      cam_info_labels = world::spawn_text(global_gui_world, global_font_name, cam_info, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 6, global_screen.h2 - 50, 0.f), IDENTITY_ROTATION);

      char buf[512];
      sprintf(buf, "%.1f\n%.1f\n%.1f\n\n\n%.1f\n%.1f\n%.1f", last_position.x, last_position.y, last_position.z
              , last_rotation.x, last_rotation.y, last_rotation.z);
      cam_info_values = world::spawn_text(global_gui_world, global_font_name, buf, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 60, global_screen.h2 - 76, 0.f), IDENTITY_ROTATION);

      cam_mode_infos = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 6, global_screen.h2 - 300, 0.f), IDENTITY_ROTATION);

      set_mode_infos();
    }

    void update(f64 dt)
    {
      if (!pad::active(global_pad_index)) return;

      if (pad::pressed(global_pad_index, PAD_KEY_7)){
        application::quit();
        return;
      }

      if (pad::pressed(global_pad_index, 3)){
        CameraMode mode = global_game_camera.get_mode();

        if (mode == CAMERA_MODE_OTRHO)
          global_game_camera.set_mode(CAMERA_MODE_FREE, .1f, 10000.f);
        else
          global_game_camera.set_mode(CAMERA_MODE_OTRHO, -1.f, 1.f);
        
        set_mode_infos();
        return;
      }

      if (pad::button(global_pad_index, 0)){
        glm::vec3 zero(0.f);
        global_game_camera.set_position(zero);
        global_game_camera.set_euler_angles(zero);
        last_rotation = last_position = zero;

        print_camera();
        return;
      }

      glm::vec3 position     = global_game_camera.get_position();
      glm::vec3 euler_angles = global_game_camera.get_euler_angles();

      // TRANSLATION: left stick
      if (pad::pressed(global_pad_index, 8)){
        global_game_camera.set_position(glm::vec3(0.f));
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
        f32 left_stick_vert  = pad::axes(global_pad_index, 1);
        if (left_stick_vert > PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_FORWARD, dt);
        }
        else if (left_stick_vert < -PAD_SENSITIVITY){
          global_game_camera.move(CAMERA_MOVE_BACKWARD, dt);
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
        global_game_camera.set_euler_angles(glm::vec3(0));
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
          f32 right_stick_vert  = pad::axes(global_pad_index, 4);
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

      bool update_print = false;

      if (last_rotation != euler_angles){
        global_game_camera.set_euler_angles(euler_angles);
        last_rotation = euler_angles;
        update_print = true;
      }

      if (last_position != position){
        global_game_camera.set_position(position);
        last_position = position;
        update_print = true;
      }

      if (update_print) print_camera();
    }

    void shutdown()
    {
      world::despawn_text(global_gui_world, projection_text);
      world::despawn_text(global_gui_world, cam_info_labels);
      world::despawn_text(global_gui_world, cam_info_values);
      world::despawn_text(global_gui_world, cam_mode_infos);

      camera_free::reset_ortho();
    }
  }
}